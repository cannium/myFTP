import socket

########################################
# you should set the right value for this part

serverAddress = '127.0.0.1'
serverPort = 21
users = { 'can':'123',
		'test':'test'}
########################################

sockets = {}
users['doesnotexist'] = 'hehe'

# create sockets
for name in users:
	print name, users[name]

	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect( (serverAddress, serverPort) )
	sockets[name] = s

# authentication
for name in sockets:
	s = sockets[name]
	print s.recv(1024)
	s.send('USER ' + name)
	print s.recv(1024)
	s.send('PASS ' + users[name])
	print name, s.recv(1024)

del users['doesnotexist']
del sockets['doesnotexist']


# mkdir
for name in sockets:
	s = sockets[name]
	s.send('MKD ' + name + 'looooogsuffix')
	print name, s.recv(1024)


# port & list

