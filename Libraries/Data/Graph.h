#ifndef BASE_GRAPH_H_
#define BASE_GRAPH_H_
#include <Auto.h>
#include <Type.h>
#include <Lock.h>

#define USE_FINITE_GRAPH_CHECK 0

namespace Base {
class Rule;

class Graph {
 public:
  ~Graph();

  /* @NOTE: by default we only assign new rule, we never support decline
   * an existed rule to keep everything simple */
  Bool Assign(Shared<Rule>&& rule, Vector<String>&& dependencies);
  Bool Assign(Shared<Rule>&& rule);
  Bool Assign(Shared<Rule>& rule, Vector<String>&& dependencies);
  Bool Assign(Shared<Rule>& rule);

  /* @NOTE: get ready rules */
  Vector<Weak<Rule>>& Ready();

  /* @NOTE: check existence of a rule */
  Bool IsExist(Shared<Rule>&& rule);
  Bool IsExist(Shared<Rule>& rule);

  /* @NOTE: notify a rule has been done an gurantee it has been clear
   * completely inside Graph */
  Bool Done(Weak<Rule> pointer);

  /* @NOTE: spin lock and unlock a circle session */
  void Lock();
  void Unlock(Bool all = False);

  /* @NOTE: access private properties of Rule */
  static Int* Stage(Graph& graph, Shared<Rule> rule);
  static Int* PlanId(Graph& graph, Shared<Rule> rule);

  /* @NOTE: access relationships of Rule */
  static Vector<Shared<Rule>>* Up(Graph& graph, Shared<Rule> rule);
  static Vector<Shared<Rule>>* Down(Graph& graph, Shared<Rule> rule);

  /* @NOTE: check infinited loop if we add the rule */
  static Bool IsInf(Graph& graph, Shared<Rule>& rule);

#if USE_FINITE_GRAPH_CHECK
  UInt MaxCheck = USE_FINITE_GRAPH_CHECKER;
#endif

 private:
  Map<String, Shared<Rule>> _Rules;
  Vector<Weak<Rule>> _Ready;
  Base::Lock _Lock;
};

class Rule: public std::enable_shared_from_this<Rule> {
 public:
  enum StageE {
    Uninitial = 0,
    Initial = 1,
    Waiting = 2,
    Running = 3,
    Finished = 4
  };

  explicit Rule(String name);
  virtual ~Rule();

  Rule();

  /* @NOTE: when a node is done and removed from Graph, it will notify
   * decendant nodes */
  Bool Done(Rule* dependency = None);

  /* @NOTE: add a new dependency */
  Bool DependBy(Shared<Rule>&& rule);
  Bool DependBy(Shared<Rule>& rule);

  /* @NOTE: get status of this rule */
  Bool IsReady();

  /* @NOTE: get information of this rule */
  Property<Bool> Lock;
  Property<UInt> Level;
  Property<String> Name;

 protected:
  /* @NOTE: how to adapt rule to actions, we will define an abstract method
   * to control it. By default, we will do nothing and only return true */
  virtual Bool Switch(Graph& graph);

  /* @NOTE: add a new rule as a new child */
  Bool WaitBy(Shared<Rule>&& rule);

 private:
  /* @NOTE: these functions will be used to provide access to graph with
   * secure checking */
  friend Int* Graph::Stage(Graph& graph, Shared<Rule> rule);
  friend Int* Graph::PlanId(Graph& graph, Shared<Rule> rule);
  friend Vector<Shared<Rule>>* Graph::Up(Graph& graph, Shared<Rule> rule);
  friend Vector<Shared<Rule>>* Graph::Down(Graph& graph, Shared<Rule> rule);

  String _Name;
  StageE _Stage;
  Vector<Shared<Rule>> _Up, _Down;
  Int _Level, _PlanId;
};
}  // namespace Base
#endif  // BASE_GRAPH_H_
