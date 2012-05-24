#!/usr/bin/python
# -*- coding: utf-8 -*-

import socket
import time
import random
import os

########################################
# you should set the right values for this part


serverAddress = '127.0.0.1'
serverPort = 21
# foramt:
# 'name' : 'password'
users = { 'can':'123',
		'test':'test'}

clientAddress = '127.0.0.1'


########################################







size = 2 ** 15
socket.setdefaulttimeout(5)

def port(controlSocket):
	clientPort = random.randint(10000, 20000)

	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.bind( (clientAddress, clientPort))
	s.listen(1)

	command = "PORT " + clientAddress.replace('.',',') + ',' + \
			   str(clientPort/256) + ',' + str(clientPort%256)
	controlSocket.send(command + '\r\n')
	print controlSocket.recv(size)
	return s


def pasv(controlSocket):
	controlSocket.send('PASV')
	data = controlSocket.recv(size)
	data = data.split(' ')[-1]
	data = data[1:-4]
	ipString = data.split(',')[0]
	ip = data.split(',')[1:4]
	for x in ip:
		ipString += '.' + x
	port = int(data.split(',')[-2]) * 256 + int(data.split(',')[-1])
	print ipString, port
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect( (ipString, port))
	return s


if __name__ == '__main__' :
	sockets = {}
	users['doesnotexist'] = 'hehe'

	print '########### authentication test ###########'
	for name in users:
		print 'name:', name, 'password:', users[name]

		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.connect( (serverAddress, serverPort) )
		sockets[name] = s

	print '------------------------------------'
	
# authentication
	for name in sockets:
		s = sockets[name]
		s.recv(size)
		s.send('USER ' + name)
		s.recv(size)
		s.send('PASS ' + users[name])
		data = s.recv(size)
		print name, '[', data[4:-2], ']'

	del users['doesnotexist']
	del sockets['doesnotexist']

	print '########### change to binary mode(optional) ###########'
	for name in sockets:
		s = sockets[name]
		s.send('TYPE I')
		data = s.recv(size)
		print name, '[', data[4:-2], ']'


	print '########### pwd, mkdir, cwd, rename ###########'
	for name in sockets:
		print ''
		print 'User: ', name
		print '------------------------------------'
		
		s = sockets[name]

		s.send('PWD')
		print 'try: pwd'
		print s.recv(size)[4:-2]

		s.send('MKD ' + name + 'looooogsuffix')
		print 'try: mkdir ', name + 'looooogsuffix'
		print s.recv(size)[4:-2]

		s.send('CWD ' + name + 'looooogsuffix')
		print 'try: cd ', name + 'looooogsuffix'
		print s.recv(size)[4:-2]

		s.send('CWD ..')
		print 'try: cd ..'
		print s.recv(size)[4:-2]

		s.send('PWD')
		print 'try: pwd'
		print s.recv(size)[4:-2]

		print 'try: rename ', name + 'looooogsuffix', name + 'shortsuffix'
		s.send('RNFR '+ name + 'looooogsuffix')
		print s.recv(size)[4:-2]
		s.send('RNTO '+ name + 'shortsuffix')
		print s.recv(size)[4:-2]


	print '########### active mode LIST ###########'
# port & list
	for name in sockets:
		s = port(sockets[name])
		sockets[name].send('LIST\r\n')
		try:
			connect, address = s.accept()
			data = connect.recv(size)
			print 'LIST format:'
			print '------------------------------------'		
			print data
			print '------------------------------------'
			print sockets[name].recv(size)
			connect.close()
		except:
			print 'Fail!!'
		s.close()
		break		# test once

	print '########### active mode STOR ###########'
	print 'upload file1.txt as test.txt'
# port & stor
	for name in sockets:
		s = port(sockets[name])
		time.sleep(1)	# bug reminder
		sockets[name].send('STOR test.txt')
		try:
			connect, address = s.accept()
			testFile = file('file1.txt')
			while True:
				data = testFile.read(size)
				if(data):
					connect.send(data)
				else:
					break
			print sockets[name].recv(size)
			connect.close()
			print sockets[name].recv(size)			  
		except:
			print 'Fail!!'
		s.close()
		break		# test once 
		

	print '########### active mode RETR ###########'
	print 'download test.txt as file2.txt'
# port & retr
	for name in sockets:
		s = port(sockets[name])
		time.sleep(1)
		sockets[name].send('RETR test.txt')
		try:
			connect, address = s.accept()
			testFile = open('file2.txt', 'w')
			while True:
				data = connect.recv(size)
				if(data):
					testFile.write(data)
				else:
					break
			print sockets[name].recv(size)
			testFile.close()
			connect.close()
		except:
			print 'Fail!!'
		s.close()
		break		# test once



	print '########### passive mode STOR ###########'
	print 'upload file1.txt as test2.txt'
# pasv & stor
	for name in	sockets:
		try:
			s = pasv(sockets[name])
			time.sleep(1)
			sockets[name].send('STOR test2.txt')
			testFile = file('file1.txt')
			while True:
				data = testFile.read(size)
				if(data):
					s.send(data)
				else:
					break
			print sockets[name].recv(size)
			s.close()
			print sockets[name].recv(size)
		except:
			print 'Fail!!'		
		break	# test once


	print '########### passive mode RETR ###########'
	print 'download test2.txt as file3.txt'
# pasv & retr
	for name in sockets:
		try:
			s = pasv(sockets[name])
			time.sleep(1)
			sockets[name].send('RETR test2.txt')
			testFile = open('file3.txt', 'w')
			while True:
				data = s.recv(size)
				if(data):
					testFile.write(data)
				else:
					break
			print sockets[name].recv(size)
			testFile.close()
			s.close()
		except:
			print 'Fail!!'
		break	# test once
		

	print '########### delete files ###########'
# delete file1, file2 in remote
	for name in sockets:
		s = sockets[name]

		s.send('DELE test.txt')
		print 'try: delete test.txt'
		print s.recv(size)[4:-2]

		s.send('DELE test2.txt')
		print 'try: delete test2.txt'
		print s.recv(size)[4:-2]

		s.send('DELE test3.txt')
		print 'try: delete test3.txt'
		print s.recv(size)[4:-2]
		break


	print '########### files compare ###########'	
# file comapre
	file1 = file('file1.txt').readlines()
	file2 = file('file2.txt').readlines()
	file3 = file('file3.txt').readlines()

	print 'file1 is the original file'
	print 'file2 is the file upload then download in active mode'
	print 'file3 is the file upload then download in passive mode'
	print '------------------------------------'		

	if(file1 == file2):
		print 'file1 == file2'
	else:
		print 'file1 != file2'

	if(file1 == file3):
		print 'file1 == file3'
	else:
		print 'file1 != file3'

