import SimpleHTTPServer
import SocketServer
import httplib
import shutil
import fix_yahoo_finance as yf
from urlparse import urlparse, parse_qs

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
        
        params = parse_qs(urlparse(request_path).query)
        stock = params['s'][0]
        month_begin = params['a'][0]
        day_begin = params['b'][0]
        year_begin = params['c'][0]
        month_end = params['d'][0]
        day_end = params['e'][0]
        year_end = params['f'][0]
        type = params['g'][0]
        
        yf.pdr_override()
        
        data_begin_str = year_begin + "-" + month_begin +"-" + day_begin
        date_end_str = year_end + "-" + month_end +"-" + day_end
        if(type == 'd'):
            action_to_request = None
        else:
            action_to_request = 'only'
        print stock, data_begin_str, date_end_str, action_to_request
        pandas_data = yf.download(stock, start = data_begin_str, end = date_end_str, actions = action_to_request)
        print pandas_data
        if('d' == type):
            data = pandas_data.to_csv(columns = ['Open','High','Low','Close','Volume','Adj Close'])
        else:
            data = pandas_data.to_csv(columns = ['action','value'])
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