import SimpleHTTPServer
import SocketServer
import httplib
import shutil
try:
	from cStringIO import StringIO
except ImportError:
	from StringIO import StringIO

class HTTPSRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
	def do_GET(self):
		"""Serve a GET request."""
		f = self.send_head()
		if f:
			self.copyfile(f, self.wfile)
			f.close()
	
	def send_head(self):
		"""Common code for GET and HEAD commands.

		This sends the response code and MIME headers.

		Return value is either a file object (which has to be copied
		to the outputfile by the caller unless the command was HEAD,
		and must be closed by the caller under all circumstances), or
		None, in which case the caller has nothing further to do.

		"""
		
		print self.path
		request_path = self.path[1:]
		
		ctype = 'application/octet-stream'
		f = StringIO()
		
		slash_location = request_path.find('/')
		
		if(-1 == slash_location):
			self.send_error(404, "File not found")
			return None
		
		c = httplib.HTTPSConnection( request_path[:slash_location] )
		c.request("GET", request_path[slash_location:] )
		response = c.getresponse()
		data = response.read()
		#print data
		f.write(data)
		self.send_response(200)
		length = f.tell()
		f.seek(0)
		self.send_header("Content-type", ctype)
		self.send_header("Content-Length", str(length))
		self.end_headers()
		return f

	def copyfile(self, source, outputfile):
		"""Copy all data between two file objects.

		The SOURCE argument is a file object open for reading
		(or anything with a read() method) and the DESTINATION
		argument is a file object open for writing (or
		anything with a write() method).

		The only reason for overriding this would be to change
		the block size or perhaps to replace newlines by CRLF
		-- note however that this the default server uses this
		to copy binary data as well.

		"""
		shutil.copyfileobj(source, outputfile)

		
PORT = 8017
Handler = HTTPSRequestHandler#SimpleHTTPServer.SimpleHTTPRequestHandler#
httpd = SocketServer.TCPServer(("127.0.0.1", PORT), Handler)


print "serving at port", PORT
httpd.serve_forever()