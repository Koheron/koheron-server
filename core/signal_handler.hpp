/// Signal handler
///
/// (c) Koheron

#ifndef __SIGNAL_HANDLER_HPP__
#define __SIGNAL_HANDLER_HPP__

namespace kserver {

class KServer;

class SignalHandler
{
  public:
    SignalHandler();
    ~SignalHandler();
    
    int Init(KServer *kserver_);
    
    int Interrupt() const {return s_interrupted;}
    
    static int volatile s_interrupted;
    static KServer *kserver;
        
  private:
    int status;
    
    int set_interrup_signals();
    int set_ignore_signals();
    int set_crash_signals();
};

} // namespace kserver

#endif // __SIGNAL_HANDLER_HPP__
