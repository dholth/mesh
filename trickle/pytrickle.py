#!/usr/bin/env python
"""
Wrap OpenMesh trickle algorithm.
"""
import attr
import heapq
import random
import pprint

from _trickle import lib as _trickle, ffi

# Slow all trickles if buffer full
GLOBAL_BACKOFF = True


def setup(i_min=200, i_max=2048, c=1):
    """
    i_min is multiplied by i_max to get the real max interval
    """
    _trickle.trickle_setup(i_min, i_max, c)


setup(
)  # floating point exception (overflow?) if timer_reset called before setup


class Trickle(object):
    """
    Instance of trickle state for some data.
    """

    I_RELATIVES = set()

    def __init__(self, packet=None):
        self.instance = ffi.new('trickle_t*')
        self.packet = packet

    def __repr__(self):
        return "Trickle" + str((self.instance.t, self.instance.i,
                                self.instance.i_doublings, self.instance.c))

    def rx_consistent(self, time_now):
        """
        Consistent value received.
        """
        _trickle.trickle_rx_consistent(self.instance, time_now)

    def rx_inconsistent(self, time_now):
        """
        Inconsistent value received.
        """
        _trickle.trickle_rx_inconsistent(self.instance, time_now)

    def timer_reset(self, time_now):
        """
        Reset interval timer. Called when associating trickle with new key.
        """
        _trickle.trickle_timer_reset(self.instance, time_now)

    def tx_register(self, time_now):
        """
        Register a successful TX on the instance
        """
        _trickle.trickle_tx_register(self.instance, time_now)

    def tx_timeout(self, time_now):
        """
        Return True if a tx is needed
        """
        out_do_tx = ffi.new("_Bool*")
        _trickle.trickle_tx_timeout(self.instance, out_do_tx, time_now)
        self.I_RELATIVES.add(self.instance.i_doublings)
        return out_do_tx[0]

    @property
    def enabled(self):
        """
        Is this trickle enabled? (Will it ever be transmitted?)
        """
        return _trickle.trickle_is_enabled(self.instance)

    @enabled.setter
    def enabled(self, value):
        if value:
            _trickle.trickle_enable(self.instance)
        else:
            _trickle.trickle_disable(self.instance)


@attr.s
class Packet(object):
    key = attr.ib()
    version = attr.ib()

    def consistent(self, packet):
        return self.key == packet.key and self.version == packet.version


@attr.s
class Event(object):
    timestamp = attr.ib(cmp=True)
    packet = attr.ib(cmp=False)
    source = attr.ib(cmp=False, default=None)


@attr.s
class EventQueue(object):
    queue = attr.ib(default=attr.Factory(lambda: []))

    def push(self, event):
        heapq.heappush(self.queue, event)

    def pop(self):
        return heapq.heappop(self.queue)


Q = EventQueue()

TIMEOUTS = 0
TX = 0
RX = 0
NEW = 0
EXISTING = 0
DISCARD = 0


@attr.s
class Node(object):
    CAPACITY = 4
    name = attr.ib()
    # keys that will not be deleted
    persist = attr.ib(default=attr.Factory(set))
    trickles = attr.ib(default=attr.Factory(lambda: {}), cmp=False)
    # nodes that we can transmit to
    adjacent = attr.ib(default=attr.Factory(list), cmp=False, repr=False)
    # interference
    blocked = attr.ib(default=attr.Factory(lambda: False))

    def step(self, time_now):
        global TIMEOUTS, TX
        if not self.trickles:
            return
        t_min = min(trickle.instance.t for trickle in self.trickles.values())
        if t_min >= time_now:
            return  # version_handler.c does this with order_update; is tx_timeout() buggy?
            # put priority queue here
        for trickle in self.trickles.values():
            if trickle.instance.t < time_now:
                TIMEOUTS += 1
            if trickle.tx_timeout(time_now):  # wait until earliest trickle t
                TX += 1
                self.blocked = True  # disallow rx while transmitting
                trickle.tx_register(time_now)
                Q.push(Event(time_now, trickle.packet, self))

    def discard(self, time_now):
        """
        Get rid of best? packet.
        """
        global DISCARD
        # worst = (0, random.choice(list(t for t in self.trickles if not t in self.persist)))
        worst = max((trickle.instance.i_doublings, trickle.packet.key)
                    for trickle in self.trickles.values()
                    if not trickle.packet.key in self.persist)
        if worst:
            DISCARD += 1
            del self.trickles[worst[1]]
        return worst

    def rx(self, event):
        global RX, NEW, EXISTING
        if event.source == self:
            return
        RX += 1
        time_now = event.timestamp
        packet = event.packet
        old_trickle = self.trickles.get(packet.key)
        if not old_trickle:
            self.trickles[packet.key] = Trickle(packet=packet)
            self.trickles[packet.key].timer_reset(0)
            if len(self.trickles) >= self.CAPACITY:
                print "evict", self.discard(time_now)
                if GLOBAL_BACKOFF:
                    for trickle in self.trickles.values():
                        trickle.rx_consistent(time_now)  # back everyone off
                        # problem, all new data will slow us down after the 'net is full
                        # maybe throw some i_max items out after an interval
            print self.name, "new <-", packet
            NEW += 1
        else:
            EXISTING += 1
            old_packet = old_trickle.packet
            if old_packet.consistent(packet):
                old_trickle.rx_consistent(time_now)
                print self.name, "got ==", packet
            else:
                old_trickle.rx_inconsistent(time_now)
                print self.name, old_trickle.packet, "neq !=", packet
                if old_trickle.packet < packet:
                    old_trickle.packet = packet


# Simulate with event queue...

# Radio module:
# Get packet (put into [1-element?] queue, with RX timestamp)
# Set packet (with TX timestamp, not too future?) Maybe unnecessary
# since all packets will be technically 'late' on transmit
# MAC - XMAC / Contiki MAC
# Event-based.
# Radio can get packet every 500us or so, if on.
# We intend to wake up every 1000us (1ms, ~SysTick?)

# Storage module
# Plenty of O(n) operations
# Iterate until a packet needs tx incidentally updating timestamps
# Enqueue with radio module
# Keep going in a circle or until n(packets) seen

STEP = 200  # does it help to be same as i_min?

if __name__ == "__main__":
    nodes = []

    log = open('where-is-f.txt', 'w+')

    for i in range(6):
        name = chr(ord('A') + i)
        node = Node(name)
        node.rx(Event(0, Packet(name, 0)))
        node.persist.add(name)
        nodes.append(node)

    nodes[0].CAPACITY = 42

    for node1, node2 in zip(nodes, nodes[1:]):
        node1.adjacent.append(node2)
        node2.adjacent.append(node1)

    last_tx = 0
    for t in range(4000, 65536 * 4, STEP):  # step by trickle i_min?
        tx_happened = False
        if Q.queue and Q.queue[0].timestamp <= t:
            event = Q.pop()
            if not tx_happened:
                print "%05d TX" % t, event
                for node in event.source.adjacent:
                    if not node.blocked:
                        node.rx(event)
                        node.blocked = True
                tx_happened = True
        for node in nodes:
            node.step(t)
            node.blocked = False
        if tx_happened:
            log.write("%06d %03d " % (t, t - last_tx))
            last_tx = t
            log.write(
                str([''.join(sorted(node.trickles.keys())) for node in nodes]))
            log.write('\n')
            print

    for node in nodes:
        print node

    log.close()

    pprint.pprint(
        list(sorted(zip(('TIMEOUTS', 'RX', 'TX', 'NEW', 'EXISTING', 'DISCARD'), (TIMEOUTS, RX, TX,
                                                          NEW, EXISTING, DISCARD)), key=lambda x: x[1])))

    pprint.pprint(Trickle.I_RELATIVES)
    pprint.pprint(list(x.bit_length() for x in Trickle.I_RELATIVES))