import libwebsock

def onopen(client):
  print "Onopen python"
  print client


def onmessage(client, msg):
  print "Client {0} sent message: {1}".format(client, msg)

libwebsock.onopen(onopen)
libwebsock.onmessage(onmessage)
libwebsock.run("3333")
