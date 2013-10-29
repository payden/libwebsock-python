import libwebsock


def onopen(client):
  print "New Client: {0}".format(client)
  print "Current conn list: {0}".format(libwebsock.connected_clients())
  print "PINGING"
  libwebsock.ping(client)

def onpong(client):
  print "Got pong!"


def onclose(client):
  print "Closing client: {0}".format(client)
  print "Current conn list: {0}".format(libwebsock.connected_clients())

def onmessage(client, msg):
  print "Client {0} sent message: {1}".format(client, msg)
  # echo message back to all clients excluding who sent it
  for c in libwebsock.connected_clients():
    if c != client:
      print "Sending message from {0} to {1}".format(client, c)
      libwebsock.send(c, msg)


libwebsock.onclose(onclose)
libwebsock.onopen(onopen)
libwebsock.onmessage(onmessage)
libwebsock.onpong(onpong)
libwebsock.run("3333")
