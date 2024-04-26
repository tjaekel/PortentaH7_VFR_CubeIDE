#/usr/bin/env python

#Python network access to DualSPIder MCU via TCP/IP
#Remark: we have to know the MCU IP address, as STATIC or via DHCP
#with DHCP - use the MCU host name as "DSMCU" + HostName from syscfg (3 characters)

import socket
import time

#define if MCU would use DHCP or not:
#if DHCP: we resolve IP address via hostname, if not,
#we use the static IP address provided here (NETBIOS does not work on STAATIC)
isDHCPUsed = False

#the STATIC IP address if DHCP is not used
defaultHostIPaddress = "192.168.0.153"

if isDHCPUsed == True:
    #we can resolve IP address by name via NETBIOS - define the MCU hostname
    defaultHostName = "dsmcutj"     #if not empty - use the name and resolve
else:
    #but on static, with direct cable, it does not work (no DHCP and DNS)
    defaultHostName = None

TCPport = 80            #the port for commands
maxTCPLen = 2*4*1024    #the maximum expected packet length (larger as largest response)

#the SPI channels
FPGA = 0
PRI = 1
AUX = 2

class NetworkInterface():
    """Our Network Object to interface MCU via TCP/IP commands"""

    def __init__(self, IPAddress=defaultHostIPaddress):
        global TCPport
        global defaultHostName

        self.host = IPAddress
        self.port = TCPport
        self.hostname = defaultHostName
        #resolve IP address by name - but just if DHCP is used (see on top)
        if self.hostname != None :
            self.host = socket.gethostbyname(self.hostname)
        self.sock = None

    def ConnectOpen(self):
        if self.sock == None:
            self.sock = socket.create_connection((self.host, self.port))
            self.sock.setblocking(1)

    def ConnectClose(self):
        if self.sock != None:
            self.sock.close()
            self.sock = None

    def TextCommand(self, cmd):
        """ATTENTION: this TextCommand() supports only up to 1460 ASCII characters total,
           including the string GET /C !
           for longer ASCII commands - use TextCommandLarge()
        """
        global maxTCPLen
        #ATT: we need 2x NEWLINE at the end! Web Server expects it as end_of_request
        message = 'GET /' + cmd + '\r\n\r\n'
        self.ConnectClose()
        self.ConnectOpen()
        self.sock.sendall(message.encode())
        data = self.sock.recv(maxTCPLen).decode()
                 
        #print response for the command
        #print("\nLen response: %d" % (len(data)))
        print(data)

        #close connection as web browser would do
        self.ConnectClose()

    def TextCommandBinary(self, cmd):
        """TextCommandLarge() send an ASCII command via BINARY: CMD 0x01 plus LEN as 3 bytes
           where LEN is the total length of following in binary, as LITTLE_ENDIAN
        """
        global maxTCPLen
        lenCmd  = len(cmd)          #without NL at the end
        #code for BINARY command (with LEN, but as ASCII text)
        message = bytearray(lenCmd + 4)
        message[0] = 0x01
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length
        for i in range(lenCmd) :
            message[4 + i] = ord(cmd[i])

        #print(repr(message))

        self.ConnectOpen()

        self.sock.sendall(message)
        data = self.sock.recv(maxTCPLen)
                 
        #print response for the command
        #print(repr(data))
        lenRsp = data[1] << 0
        lenRsp = lenRsp | (data[2] << 8)
        lenRsp = lenRsp | (data[3] >> 16)
        #it should be enough to use len(data)>4 : - just if we have non-empty response
        if lenRsp > 0 :
            print(data[4:].decode())
    
        #leave the connection open - IT DOES NOT WORK
        self.ConnectClose()

    def CommandShell(self):
        cmd = ""

        self.ConnectOpen()
        while cmd != 'q':
            cmd = input("-> ")
            if cmd == 'q':
                break;
            #self.TextCommand(cmd)
            self.TextCommandBinary(cmd)

        #close the connection when done
        self.ConnectClose()
        print ('Good bye')

    def SPITransaction(self, spipkt):
        global maxTCPLen
        """send a binary raw SPI transaction to PRI or AUX channel
           ATTENTION: the max. binary length is limited to 8184
        """
        lenCmd = len(spipkt)
        message = bytearray(lenCmd + 4)

        message[0] = 0x02
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length
        #copy the SPI transaction bytes
        for i in range(lenCmd) :
            message[4 + i] = spipkt[i] & 0xFF

        print("Len: %d" % (lenCmd))

        #DEBUG
        #print(repr(message))

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        data = b''
        while len(data) < (lenCmd + 4) :
            data = data + self.sock.recv(maxTCPLen)

        #option: we could evaluate the length in response, should be the same
        l = data[1] << 0
        l = l | data[2] << 8
        l = l | data[3] << 16
        print("len rx: %d" % (l))
        
        #let the connection open - IT DOES NOT WORK
        self.ConnectClose()
        return data[4:]

    def DummyTransaction(self, spipkt):
        global maxTCPLen
        """send a binary raw SPI transaction to PRI or AUX channel
           ATTENTION: the max. binary length is limited to 8184
        """
        lenCmd = len(spipkt)
        message = bytearray(lenCmd + 4)

        message[0] = 0x10
        message[1] = (lenCmd >>  0) & 0xFF
        message[2] = (lenCmd >>  8) & 0xFf
        message[3] = (lenCmd >> 16) & 0xFF  #LITTLE ENDIAN length

        #send as command
        self.ConnectOpen()

        self.sock.sendall(message)
        
        #we have to loop: recv will return just max. 2140 bytes
        data = b''
        while len(data) < (lenCmd + 4) :
            data = data + self.sock.recv(maxTCPLen)

        #let the connection open - IT DOES NOT WORK
        self.ConnectClose()

def Main():
    global defaultHostIPaddress
    global TCPport
    
    mcuNet = NetworkInterface()

    if 0 :
        maxTCPlen = (64*1024 + 16)
        b = bytearray(maxTCPlen)
        for i in range(maxTCPlen // 4):
            b[0 + i*4] = 0x02
            b[1 + i*4] = 0xE0
            b[2 + i*4] = 0x11
            b[3 + i*4] = 0x22

        start_time = time.time()
        r = mcuNet.DummyTransaction(b)
        print("--- %s seconds ---" % (time.time() - start_time))

        start_time = time.time()
        r = mcuNet.SPITransaction(b)
        print("--- %s seconds ---" % (time.time() - start_time))

        print("resp len: %d | %d" % (maxTCPlen, len(r)))

        if 0 :
            for i in range(len(r)):
                print("%2.2x " % (r[i]), end='')
            print()

    if 1 :
        mcuNet.TextCommand("help")
        mcuNet.TextCommandBinary("help")
        mcuNet.CommandShell()

if __name__ == '__main__':
    Main()

