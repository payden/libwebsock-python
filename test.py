import libwebsock

def onopen(client):
  print "Got new client: {0}".format(client)
  print "Socket: {0}".format(client.sock)


def onmessage(client, msg):
  print "Client {0} sent message: {1}".format(client, msg)
  # echo message back
  libwebsock.send(client, msg)

libwebsock.onopen(onopen)
libwebsock.onmessage(onmessage)
libwebsock.run("3333")
