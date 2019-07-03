import multiprocessing
import queue


class QueueMsgBus:

    NOT_REGISTERED = None

    def __init__(self, queue_to_use, module_id=NOT_REGISTERED):
        self.msg_queue = queue_to_use
        self.id = module_id

    def try_get(self):
        assert self.id != self.NOT_REGISTERED
        try:
            msg = self.msg_queue.get(block=False)
            if msg[0] == self.id:
                return msg[1]
            else:
                self._reput(msg)
                return None
        except queue.Empty:
            return None

    def _reput(self, msg):
        self.msg_queue.put(msg)

    def put_new(self, destination_id, msg):
        self.msg_queue.put( (destination_id, msg) )

    def put_new_to_itself(self, msg):
        self.msg_queue.put( (self.id, msg) )