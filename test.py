import libwebsock
import gc

def onopen(client):
  print "New Socket: {0}".format(client.sock)
  print "Address: {0}".format(client.addr)

def onclose(client):
  print "Closing socket: {0}".format(client.sock)

def onmessage(client, msg):
  print "Client {0} sent message: {1}".format(client, msg)
  # echo message back
  libwebsock.send(client, msg)


libwebsock.onclose(onclose)
libwebsock.onopen(onopen)
libwebsock.onmessage(onmessage)
libwebsock.run("3333")
