#include <Common/SimpleLog.h>

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

class SimpleLog::Impl {
  public:

  // constructor
  Impl();
  
  // destructor
  ~Impl();

  // list of possible message severity levels  
  enum Severity {Info,Error,Warning}; 

  // base log function, with printf-like format arguments
  // \param severity  Message severity.
  // \param message   Message content, printf-like format.
  // \param ap        Variable list of arguments associated with message.
  int logV(SimpleLog::Impl::Severity severity, const char *message, va_list ap);
  
  protected: 
  FILE *fp; // descriptor to be used. If NULL, using stdout/stderr.
  int formatOptions;
  int fdStdout;
  int fdStderr;

  friend class SimpleLog;
};

SimpleLog::Impl::Impl() {
  fp=NULL;
  formatOptions =   SimpleLog::FormatOption::ShowTimeStamp
                  | SimpleLog::FormatOption::ShowSeveritySymbol
                  | SimpleLog::FormatOption::ShowMessage;
  fdStdout = fileno(stdout);
  fdStderr = fileno(stderr);
}

SimpleLog::Impl::~Impl() {
  if (fp!=NULL) {
    fclose(fp);
    fp=NULL;
  }
}


int SimpleLog::Impl::logV(SimpleLog::Impl::Severity s, const char *message, va_list ap)
{
  char buffer[1024] = "";
  size_t len = sizeof(buffer) - 2;
  size_t ix = 0;

  if (formatOptions & SimpleLog::FormatOption::ShowTimeStamp) {  
    // timestamp (microsecond)
    struct timeval tv;
    double fullTimeNow=-1;
    if(gettimeofday(&tv,NULL) == -1){
      fullTimeNow = time(NULL);
    } else {
      fullTimeNow = (double)tv.tv_sec + (double)tv.tv_usec/1000000;
    }
    time_t now;
    struct tm tm_str;
    now = (time_t)fullTimeNow;
    localtime_r(&now, &tm_str);
    double fractionOfSecond=fullTimeNow-now;
    ix+=strftime(&buffer[ix], len-ix, "%Y-%m-%d %T", &tm_str);
    char str_fractionOfSecond[10];
    snprintf(str_fractionOfSecond,sizeof(str_fractionOfSecond),"%.6lf",fractionOfSecond);
    ix+=snprintf(&buffer[ix], len-ix, ".%s",&str_fractionOfSecond[2]);
    if (ix>len) { ix=len; }
  }

  if (formatOptions & SimpleLog::FormatOption::ShowSeveritySymbol) {
    if (s==Severity::Error) {
      ix+=snprintf(&buffer[ix], len-ix, " !!! ");
    } else if (s==Severity::Warning) {
      ix+=snprintf(&buffer[ix], len-ix, "  !  ");
    } else {
      ix+=snprintf(&buffer[ix], len-ix, "     ");
    }
  }

  if (formatOptions & SimpleLog::FormatOption::ShowSeverityTxt) {
    if (s==Severity::Error) {
      ix+=snprintf(&buffer[ix], len-ix, "Error - ");
    } else if (s==Severity::Warning) {
      ix+=snprintf(&buffer[ix], len-ix, "Warning - ");
    } else {
      //ix+=snprintf(&buffer[ix], len-ix, "");
    }
  }

  
  if (formatOptions & SimpleLog::FormatOption::ShowMessage) {
   ix+=vsnprintf(&buffer[ix], len-ix, message, ap);
   if (ix>len) { ix=len; } 
  }

  buffer[ix] = '\n';
  ix++;
  buffer[ix]=0;

  int fd;
  if (fp != NULL) {
    fd = fileno(fp);
  } else {
    if (s==Severity::Error) {
      fd = fdStderr;
    } else {
      fd = fdStdout;
    }
  }
  write(fd, buffer, ix);

  return 0;
}






SimpleLog::SimpleLog(const char* logFilePath) {
  pImpl=std::make_unique<SimpleLog::Impl>();
  if (pImpl==NULL) { throw __LINE__; }
  setLogFile(logFilePath);
}

SimpleLog::~SimpleLog() {
  setLogFile(NULL);
}

int SimpleLog::setLogFile(const char* logFilePath) {
  if (pImpl->fp!=NULL) {
    fclose(pImpl->fp);
    pImpl->fp=NULL;
  }
  if (logFilePath!=NULL) {
    pImpl->fp=fopen(logFilePath,"a");
    if (pImpl->fp==NULL) {
      return -1;
    }
  }
  return 0;
}


int SimpleLog::info(const char *message, ...)
{
  int err = 0;

  va_list ap;
  va_start(ap, message);
  err = pImpl->logV(Impl::Severity::Info,message, ap);
  va_end(ap);

  return err;
}

int SimpleLog::error(const char *message, ...)
{
  int err = 0;

  va_list ap;
  va_start(ap, message);
  err = pImpl->logV(Impl::Severity::Error,message, ap);
  va_end(ap);

  return err;
}

int SimpleLog::warning(const char *message, ...)
{
  int err = 0;

  va_list ap;
  va_start(ap, message);
  err = pImpl->logV(Impl::Severity::Warning,message, ap);
  va_end(ap);

  return err;
}

void SimpleLog::setOutputFormat(int opts) {
  pImpl->formatOptions=opts;
}

void SimpleLog::setFileDescriptors(int fdStdout, int fdStderr)
{
  pImpl->fdStdout = fdStdout;
  pImpl->fdStderr = fdStderr;
}

/// \todo: thread to flush output every 1 second
