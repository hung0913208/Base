#ifndef BASE_MONITOR_H_
#define BASE_MONITOR_H_
#include <Auto.h>
#include <List.h>
#include <Type.h>
#include <Logcat.h>

#if __cplusplus
namespace Base {
class Monitor {
 public:
  enum ActTypeE {
    EAppend = 0,
    EModify = 1,
    EFind = 2,
    ERemove = 3
  };

  enum TypeE {
    EIOSync = 0,
    EPipe = 1,
    ETimer = 2
  };

  enum StatusE {
    EOffline = 0,
    EStarting = 1,
    EStarted = 2,
    EStopping = 3,
    EStopped = 4
  };

  using Check = Function<ErrorCodeE(Auto, Auto&, Int)>;
  using Perform = Function<ErrorCodeE(Auto, Auto&)>;
  using Indicate = Function<Perform*(Auto&, Int)>;
  using Fallback = Function<ErrorCodeE(Auto fd)>;

  virtual ~Monitor();

  /* @NOTE: these methods are used to append, modify or remove a fd */
  ErrorCodeE Append(Auto fd, Int mode);
  ErrorCodeE Modify(Auto fd, Int mode);
  ErrorCodeE Find(Auto fd);
  ErrorCodeE Remove(Auto fd);

  /* @NOTE: this method is used register a trigger when fd passes a
   * certain condition */
  ErrorCodeE Trigger(Auto event, Perform perform);
  void Heartbeat(Fallback fallback);

  /* @NOTE: this method is used to check status of the system */
  ErrorCodeE Status(String name);

  /* @NOTE: these methos are use to start monitor to run a certain step */
  ErrorCodeE Loop(Function<Bool(Monitor&)> status, Int timeout = 0);
  ErrorCodeE Loop(UInt steps = 1, Int timeout = 0);

  /* @NOTE: this method is call by HEAD monitor to probe fd's events */
  ErrorCodeE Scan(Auto fd, Int mode, Vector<Pair<Monitor*, Perform*>>& callbacks);

  /* @NOTE: this function is used to claim a new job should be done later before
   * the monitor is destroyed */
  ErrorCodeE Claim(Bool safed = True);

  /* @NOTE: this function is used to notify the head that the child has done a
   * job so we should decrease the job counter right now */
  Bool Done();

  /* @NOTE: this function attach a Monitor to the system so it can handle
   * requests from HEAD */
  Bool Attach(UInt retry = 5);

  /* @NOTE: this function detach a Monitor from the system so it can't handle
   * requests from HEAD */
  Bool Detach(UInt retry = 5);

  /* @NOTE: this function allow to read the Monitor status */
  UInt State();

  /* @NOTE: this function allow to access the share memories among Monitor 
   * within the same type */
  Void* Context();

  /* @NOTE: access the head monitor, they are defined as a single tone and should
   * be the latest remover */
  Monitor* &Head();

  /* @NOTE: this function is used to switch state to newer state */
  ErrorCodeE SwitchTo(UInt state);
  /* @NOTE: this function is the only way to create a monitor. With this
   * design, there is no way to create a monitor without diferent type */
  static Shared<Monitor> Make(String name, UInt type);

  /* @NOTE: this function is used to register a Monitor's builder to help to
   * build a new kind of Monitor */
  static Bool Sign(UInt type, Bool (*function)(String, UInt, Monitor**));

  /* @NOTE: this function is used to check if the type is supported or not 
   * by the system */
  static Bool IsSupport(UInt type);

  /* @NOTE: these functions help to lock and unlock a monitor system to perform
   * some actions synchronously */
  static void Lock(UInt type);
  static void Unlock(UInt type);

  /* @NOTE: acces the head monitor, they are defined as a single tone and should
   * be the latest remover */
  static Monitor* Head(UInt type);

  /* @NOTE: this function helps to check if the sample is the head recently */
  static Bool IsHead(UInt type, Monitor* sample);

 protected:
  /* @NOTE: this function is used to registry an indicator which is used 
   * to generate a Perform. By default, indicator is always activated when
   * its checker accepts to open a route. When it happens, we will receive
   * a callback Perform and we will use Reroute to redirect the callback to
   * a place for performing */
  void Registry(Check check, Indicate indicate);

  /* @NOTE: this function is used to reroute a callback to specific place for
   * calling. How to handle these callbacks? how to keep balancing these
   * callbacks? most of them has been managed by Monitor and i will provide
   * convenient virtual methods to configure them */
  ErrorCodeE Reroute(Monitor* child, Auto fd, Perform& callback);

  /* @NOTE: this function is used to check if the fd is on fallback state which
   * lead to kill and remove its tasks */
  ErrorCodeE Heartbeat(Auto fd);

  /* @NOTE: these virtual methods are used to interact with monitor */
  virtual ErrorCodeE _Append(Auto fd, Int mode);
  virtual ErrorCodeE _Modify(Auto fd, Int mode);
  virtual ErrorCodeE _Find(Auto fd);
  virtual ErrorCodeE _Remove(Auto fd);

  /* @NOTE: this virtual method is used to register a triggering with
   * specific events */
  virtual ErrorCodeE _Trigger(Auto event, Perform perform) = 0;

  /* @NOTE: this virtual method is used to route a callback to the place and it
   * can be picked by _Handle to process */
  virtual ErrorCodeE _Route(Auto fd, Perform& callback) = 0;

  /* @NOTE: this virtual method is used to check status of system or a specific
   * fd whihch is installed into the monitor */
  virtual ErrorCodeE _Status(String name, Auto fd) = 0;

  /* @NOTE: this virtual method is used to interact with lowlevel */
  virtual ErrorCodeE _Interact(Monitor* child, Int timeout,
                               Int backlog = 100) = 0;

  /* @NOTE: this virtual method is used to handle callbacks which are throw
   * from HEAD to be executed in CHILD */
  virtual ErrorCodeE _Handle(Monitor* child, Int timeout,
                             Int backlog = 100) = 0;

  /* @NOTE: this virtual method is used to access context of fd */
  virtual Auto& _Access(Auto fd) = 0;

  /* @NOTE: this virtual method is used to flush every jobs which are pending 
   * inside the Monitor  */
  virtual void _Flush() = 0;

  /* @NOTE: this virtual method is used to clean memory which are registered
   * during construsting inheritance objects */
  virtual Bool _Clean() = 0;

  /* @NOTE: by default we will lock constructors and destructor to prevent
   * user to create a new monitor with uncompatible type */
  explicit Monitor(String name, UInt type);

  /* @NOTE: these provide basic information of this monitor */
  String _Name;
  UInt _Type;

  /* @NOTE: this is used to share memory among Monitor inside a type */
  Void* _Shared;

  /* @NOTE: these are used to make links between Head and its children */
  Monitor **_PNext, **_PLast, *_Head, *_Last, *_Next, *_Prev;

  /* @NOTE: these should be managed by CHILD */
  Vector<Check> _Checks;
  Vector<Fallback> _Fallbacks;
  Map<ULong, Indicate> _Indicators;

  /* @NOTE: this variable is used to indicate if the child is fully started */
  UInt _State;

 private:
  /* @NOTE: this is the implementation of method Scan and it can be use to scan
   * and pick callbacks on single thread only */
  ErrorCodeE ScanImpl(Auto fd, Int mode, Bool heading,
                      Vector<Pair<Monitor*, Perform*>>& callbacks);

  /* @NOTE: this function is used to scan throught all indicators and return
   * the next Monitor if it has, this is not thread-safe function */
  ErrorCodeE ScanIter(Auto fd, Int mode, Monitor** next,
                      Vector<Pair<Monitor*, Perform*>>& callbacks);

  struct Action {
    ActTypeE type;
    Auto fd; Int mode;

    Action(ActTypeE type, Auto fd, Int mode);
    Action(ActTypeE type, Auto fd);
  };

  Int _Index;
  Int _Using;
  Bool _Attached, _Detached;
  Vector<Action> _Pipeline;
};
} // namespace Base
#else
#endif // __cplusplus
#endif // BASE_MONITOR_H_
