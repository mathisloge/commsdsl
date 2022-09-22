import os
import sys
import unittest

sys.path.append(os.getcwd())

import test10

class MsgHandler(test10.frame_Frame_Handler):

    def __init__(self, testObj = None):
        test10.frame_Frame_Handler.__init__(self)
        self.testObj = testObj

    def handle_message_Msg1(self, msg):
        self.msg1 = True
        if (self.testObj is not None):
            self.testObj.msg1 = test10.message_Msg1(msg)

    def handle_message_Msg2(self, msg):
        self.msg1 = True
        if (self.testObj is not None):
            self.testObj.msg2 = test10.message_Msg2(msg)            

    def handle_Message(self, msg):
        sys.exit("shouldn't happen")

class TestProtocol(unittest.TestCase):
    def setUp(self): 
        self.msg1 = None
        self.msg2 = None

    def test_1(self):
        m = test10.message_Msg2()
        m.field_f1().field_year().setValue(2127)
        m.field_f1().field_month().setValue(1)
        m.field_f1().field_day().setValue(10)

        frame = test10.frame_Frame()
        buf = frame.writeMessage(m)

        h = MsgHandler(self)
        frame.processInputData(buf, h)

        self.assertTrue(test10.eq_message_Msg2(self.msg2, m))

if __name__ == '__main__':
    unittest.main()


