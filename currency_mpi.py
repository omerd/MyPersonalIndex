import SimpleHTTPServer
import SocketServer
import requests
import datetime
import shutil
import time
import urllib2
import json
import threading
from dateutil import parser
from urlparse import urlparse, parse_qs
import fix_yahoo_finance as yf

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO
    
#TODO: synchronization issues here..
    

class CurrencyData(object):
    def __init__(self):
        self.Currencies = {}
        
    
    def getCurrencyData(self, curr_from, curr_to, date = None): #0 = today
        trials = 10
        if not date:
            date = datetime.datetime.today()
        if date > datetime.datetime.today():
            return 0
        while trials > 0:
            try:
                return self.Currencies[date.date()][curr_from][curr_to]
            except KeyError:
                trials -= 1
                date = date - datetime.timedelta(days = 1)
        
        '''try:
                #If you return values from here, they are going to be stored in the SQlite DB.
                #if curr_from == 'ILS':
                #    return 0.28
                #else:
                #    return 3.5
        except KeyError:
            pass
        return 0'''
    
    def download_currency_quotes(self, startAt, currency = 'ILS'):
        endAt = datetime.date.today()
        delta = datetime.timedelta(days = 100)
        while endAt > startAt:
            startDate = endAt - delta
            if startDate < startAt:
                startDate = startAt
            
            pandas_data = yf.download(currency + "=X", start = startDate, end = endAt)
            quotes = pandas_data['Close']
            dates = quotes.keys()
            for i in xrange(len(quotes)):
                quote_date = dates[i].date()
                quote_value = quotes[i]
                
                if not self.Currencies.has_key(quote_date):
                    self.Currencies[quote_date] = {}
                
                if not self.Currencies[quote_date].has_key('USD'):
                    self.Currencies[quote_date]['USD'] = {}
                    
                if not self.Currencies[quote_date].has_key(currency):
                    self.Currencies[quote_date][currency] = {}
                    
                self.Currencies[quote_date]['USD'][currency] = quote_value
                self.Currencies[quote_date][currency]['USD'] = 1 / quote_value
            endAt -= delta
        print 'downloaded'
    def _downloader(self):
        today = datetime.date.today()
        if today > self.last_downloaded:
            try:
                self.download_currency_quotes(self.last_downloaded)
                self.download_currency_quotes(self.last_downloaded, 'EUR')
                self.last_downloaded = datetime.date.today()
            except:
                print 'not downloaded at: ' + str(datetime.datetime.now()) #try again later
    def downloader(self):
        startDate = datetime.date(2017,1,1)
        self.last_downloaded = startDate
        yf.pdr_override()
        try:
            print 'ILS'
            self.download_currency_quotes(startDate)
            print 'EUR'
            self.download_currency_quotes(startDate, 'EUR')
            self.last_downloaded = datetime.date.today()
        except:
            print 'not downloaded at: ' + str(datetime.datetime.now()) #try again later
        while True:
            self._downloader()
            time.sleep(60*60)
    
currencyDataManager = CurrencyData()
    
    
class YahooFinanceRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):
        """Serve a GET request."""
        f = self.send_head()
        if f:
            self.copyfile(f, self.wfile)
            f.close()
    def getResponse(self,date, currency_to, currency_from):
        f = StringIO()
        val = currencyDataManager.getCurrencyData(currency_from, currency_to, parser.parse(date))
        print str(val)
        f.write(str(val))
        return f
    
    def send_head(self):
        """Common code for GET and HEAD commands.

        This sends the response code and MIME headers.

        Return value is either a file object (which has to be copied
        to the outputfile by the caller unless the command was HEAD,
        and must be closed by the caller under all circumstances), or
        None, in which case the caller has nothing further to do.

        """
        #print self.path
        date = None
        currency_to = None
        currency_from = None
        try:
            params = parse_qs(urlparse(self.path).query)
            #print params
            date = params['date'][0]
            currency_to = params['currency_to'][0]
            currency_from = params['currency_from'][0]
        except:
            self.send_error(404, "File not found")
            return None
            
                
        ctype = 'application/octet-stream'
        f = self.getResponse(date, currency_to, currency_from)
        self.send_response(200)
        length = f.tell()
        f.seek(0)
        self.send_header("Content-type", ctype)
        self.send_header("Content-Length", str(length))
        #self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
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
        
        
PORT = 8016
Handler = YahooFinanceRequestHandler#SimpleHTTPServer.SimpleHTTPRequestHandler#
httpd = SocketServer.TCPServer(("127.0.0.1", PORT), Handler)


threading.Thread(target = currencyDataManager.downloader).start()
print "serving at port", PORT
httpd.serve_forever()