import SimpleHTTPServer
import SocketServer
import requests
import datetime
import shutil
import time
import threading
try:
	from cStringIO import StringIO
except ImportError:
	from StringIO import StringIO
#TODO: synchronization issues here..
	
	
class DataFiles(object):
	def __init__(self):
		self.Fund = {}
		self.Securities = {}
		self.dates = []
	
	def getFundData(index = 0): #0 = newest
		return self.Fund[self.dates[index]]
		
	def getSecurityData(index = 0): #0 = newest
		return self.Securities[self.dates[index]]
	
	def get_sorted_days(self):
		today = datetime.date.today()
		for i in xrange(31):
			date = today - datetime.timedelta(days= i * 1)
			day = date.day
			day_str = str(day).zfill(2)
			url = 'https://www.tase.co.il/_layouts/Tase/Public/TargetFolder/0024'+day_str+'01.tas'
			result = requests.get(url, verify=False)
			if result.ok:
				date_of_data = result.content[8:14]
				_year = int('20' + date_of_data[0:2])
				_month = int(date_of_data[2:4])
				_day = int(date_of_data[4:6])
				self.dates.append(datetime.datetime(year = _year, month = _month, day = _day))
		if(len(self.dates) == 0):
			raise Exception("a")
		self.dates.sort().reverse()
			
	def get_stock_names(self,day_str):
		url = 'https://www.tase.co.il/_layouts/Tase/Public/TargetFolder/0803'+day_str+'01.tas'
		result = requests.get(url, verify=False)
		if not result.ok:
			return
		
		return result.content
		
	def get_data_fund_file(self, day_str):
		
		#addiontal info can be found in https://www.tase.co.il/_layouts/Tase/Public/TypeDesc/tase0068.def
		#but it's irrelevant in comparison to yahoo's information
		data = ''
		for i in xrange(1,3):
			url = 'https://www.tase.co.il/_layouts/Tase/Public/TargetFolder/0024'+day_str+str(i).zfill(2)+'.tas'
			result = requests.get(url, verify=False)
			if result.ok:
				data = data + result.content
				break
				
		return data

	def get_data_other_securities_file(self, day_str):
		
		#addiontal info can be found in https://www.tase.co.il/_layouts/Tase/Public/TypeDesc/tase0068.def
		#but it's irrelevant in comparison to yahoo's information
		data = ''
		for i in xrange(1,3):
			url = 'https://www.tase.co.il/_layouts/Tase/Public/TargetFolder/0542'+day_str+str(i).zfill(2)+'.tas'
			result = requests.get(url, verify=False)
			if result.ok:
				data = data + result.content
				break
		return data
		
	def download_files(self):
		try:
			#won't work when the computer is offline
			self.get_sorted_days()
		except:
			'exception thrown'
		
		try:
			self.stock_names = self.get_stock_names(str(self.dates[0].day).zfill(2))
			for i in self.dates:
				self.Fund[i] = self.get_data_fund_file(str(i.day).zfill(2))
				self.Securities[i] = self.get_data_other_securities_file(str(i.day).zfill(2))
		except:
			print 'error in get stock'
		print 'downloaded'

	def downloader(self):
		self.download_files()
		last_downloaded = datetime.datetime.now()
		while True:
			today = datetime.datet.today()
			now = datetime.datetime.now()
			if today > last_downloaded.date():
				self.download_files()
				last_downloaded = datetime.datetime.now()
			if today == last_downloaded.date() and now.hour > 21 and last_downloaded.hour < 21:
				self.download_files()
				last_downloaded = datetime.datetime.now()
			time.sleep(20*60)
	
dataFilesManager = DataFiles()
	
	
	
	
MUTUAL_FUND = 1
OTHERS = 2
	
class MutualFundQuote(object):
	def __init__(self, quote_str):
		self.RecordType = quote_str[0:2]
		self.ID = quote_str[2:10]
		self.FundType = quote_str[10:12]
		self.Sector = quote_str[12:14]
		self.SubSector = quote_str[14:16]
		self.TradeType = quote_str[16:18]
		self.Price = str(float(quote_str[18:27])/100)#.lstrip('0') 
		self.PriceType = quote_str[27:29]
		self.PriceTrend = quote_str[29]
		self.PriceChange = str(float(quote_str[30:37])/100) #lstrip('0') <--will cause trouble when the change is 00000
		self.DateOfPreviousPrice = quote_str[37:45]
		self.PreviousPrice = str(float(quote_str[45:54])/100) #lstrip('0')
		self.PreviousPriceUpdateDate = quote_str[54:62]
		self.Filler = quote_str[62:66]
		self.LastPriceDate = quote_str[66:72]
		self.LastPriceUpdateDate = quote_str[72:80]
		
class SecurityQuote(object):
	def __init__(self, quote_str):
		self.RecordType = quote_str[0:2]
		self.ID = quote_str[2:10]
		
		if '02' == self.RecordType:
			self.ClosingPrice = str(float(quote_str[10:19]))#.lstrip('0')
			self.ClosingPricesType = quote_str[19:21]
			self.ClosingPricesTrend = quote_str[21:22]
			self.PriceChange = str(float(quote_str[22:30])) #.lstrip('0')
			self.PercentageChange = str(float(quote_str[30:34]+'.'+quote_str[34:36]))
			self.ExCode = quote_str[36:38]
			self.TURNOVER_QUANTITY  = str(float(quote_str[38:49]))
			self.TURNOVER_MONETARY_VALUE = str(float(quote_str[49:60]))
			self.NUMBER_OF_TRANSACTIONS = str(float(quote_str[60:65]))
			self.Filler = quote_str[65:80]
		elif '03' == self.RecordType:
			self.OpeningPrice = str(float(quote_str[10:19]))
			self.ClosingYield = str(float(quote_str[19:24]))
			self.YieldIndicator = quote_str[24:25]
			self.ClosingAuctionPrice = str(float(quote_str[25:34]))
			self.HighestPrice = str(float(quote_str[34:43]))
			self.LowesetPrice = str(float(quote_str[43:52]))
			self.Filler = quote_str[52:80]
		
		
class YahooFinanceRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
	def do_GET(self):
		"""Serve a GET request."""
		f = self.send_head()
		if f:
			self.copyfile(f, self.wfile)
			f.close()
	
	def convert_date(self,date):
		if 8 == len(date):
			year = date[0:4]
			month = date[4:6]
			day = date[6:8]
			return str(year + '-' + month+'-'+day)
		else:
			year = date[0:2]
			month = date[2:4]
			day = date[4:6]
			return str('20'+year + '-' + month +'-'+day )

	def get_stock_name(self,security_id):
		content = dataFilesManager.stock_names
		index = content.find(security_id)
		return content[index+17:index+32]
	
	def getSecurityType(self,security_id):
		content = dataFilesManager.stock_names
		index = content.find(security_id.zfill(8))
		type = content[index+58:index+58+4]  # when the line is reversed - [::-1][16:20][::-1]
		if type == '1001':
			return MUTUAL_FUND
		else:
			return OTHERS

	def getMutualFundResponse(self, security_id,dataFile):
		if dataFile == '':
			self.send_error(404, "File not found")
			return None
		dataFile = dataFile.splitlines()
		
		relevant = []
		
		NA = 'N/A'
		#24 is the calculated Price, 25 is the BID and 23 is the Offer.
		for i in dataFile:
			q = MutualFundQuote(i)
			if q.ID == security_id.zfill(8):
				if q.PriceType == '24' :
					if(0 != len(relevant) and MutualFundQuote(relevant[0]).RecordType == '02'):
						del relevant[0]
					relevant.append(i)
		
		if len(relevant) == 0:
			self.send_error(404, "File not found")
			return None
			
		P = ','
		quote = MutualFundQuote(relevant[0])
		
		
		result =self.convert_date(quote.LastPriceDate) + P\
				+str(float(quote.PreviousPrice)/100) +P\
				+str(float(quote.Price)/100) +P\
				+str(float(quote.Price)/100) +P\
				+str(float(quote.Price)/100) +P\
				+str(1) + P\
				+str(float(quote.Price)/100) +P\
				+"\r\n"
				
		return result
	
	def getOtherSecutitiesResponse(self, security_id, data):
		if data == '':
			self.send_error(404, "File not found")
			return None
		r = data.splitlines()
		
		NA = 'N/A'
		quote02 = None
		quote03 = None
		for i in r:
			q = SecurityQuote(i)
			if q.ID == security_id.zfill(8):
				if '02' == q.RecordType: #03 - on the day data
					quote02 = SecurityQuote(i)
				elif '03' == q.RecordType: #03 - retroactive data
					quote03 = SecurityQuote(i)
		if None == quote02 or None == quote03:
			self.send_error(404, "File not found")
			return None
		
		P = ','
		date_of_data = data[8:14]
		result = self.convert_date(date_of_data) +  P\
				+str(float(quote03.OpeningPrice)/10000) +P\
				+str(float(quote03.OpeningPrice)/10000) +P\
				+str(float(quote03.OpeningPrice)/10000) +P\
				+str(float(quote02.ClosingPrice)/10000) +P\
				+str(1) + P\
				+str(float(quote02.ClosingPrice)/10000) +P\
				+"\r\n"
		return result
		
	def _getMutualFundResponse(self, security_id):
		f = StringIO()
		result = 'Date,Open,High,Low,Close,Volume,Adj Close\n'
		for i in dataFilesManager.dates:
			result += self.getMutualFundResponse(security_id, dataFilesManager.Fund[i])
		f.write(result)
		return f
	def _getOtherSecutitiesResponse(self, security_id):
		f = StringIO()
		result = 'Date,Open,High,Low,Close,Volume,Adj Close\n'
		for i in dataFilesManager.dates:
			result += self.getOtherSecutitiesResponse(security_id, dataFilesManager.Securities[i])
		f.write(result)
		return f
		
	def getResponse(self,security_id):
		if MUTUAL_FUND == self.getSecurityType(security_id):
			return self._getMutualFundResponse(security_id)
		else:
			return self._getOtherSecutitiesResponse(security_id)
	
	def send_head(self):
		"""Common code for GET and HEAD commands.

		This sends the response code and MIME headers.

		Return value is either a file object (which has to be copied
		to the outputfile by the caller unless the command was HEAD,
		and must be closed by the caller under all circumstances), or
		None, in which case the caller has nothing further to do.

		"""
		print self.path
		security_id = None

		try:
			security_id = self.getStockNumber(self.path)
		except:
			self.send_error(404, "File not found")
			return None
			
		if False == security_id.isdigit() :
			# redirect browser - doing basically what apache does
			'''self.send_response(301)
			newLocation = "http://download.finance.yahoo.com" + self.path
			self.send_header("Location", newLocation)
			self.end_headers()
			return None'''
			f = StringIO()
			result = requests.get("http://download.finance.yahoo.com" + self.path)
			f.write(result.content)
			return f
		
		ctype = 'application/octet-stream'
		#try:
		f = self.getResponse(security_id)
		
		#except:
		#	self.send_error(404, "File not found")
		#	return None
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
		
	def getStockNumber(self, path):
		start = path.find('?s=')
		end = path.find('.TA&')
		return path[start+3:end]
		
PORT = 8015
Handler = YahooFinanceRequestHandler#SimpleHTTPServer.SimpleHTTPRequestHandler#
httpd = SocketServer.TCPServer(("127.0.0.1", PORT), Handler)


threading.Thread(target = dataFilesManager.downloader).start()
print "serving at port", PORT
httpd.serve_forever()