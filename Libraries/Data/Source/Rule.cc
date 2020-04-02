#include <Graph.h>

namespace Base {
Rule::Rule(String name):
  Level{
    [&]() -> UInt&{ return *((UInt*)(&_Level)); },
    [&](UInt value){
      if (IsReady()) {
        throw Except(EBadAccess, "change rule\'s level when it\'s ready");
      } else {
        _Level = value;
      }
    }
  },
  Name{
    [&]() -> String&{ return _Name; },
    [&](String value){
      if (_Name.size() > 0) {
        throw Except(ENoSupport, "Don\'t support rename rule");
      } else {
        _Name = value;
      }
    }
  },
  _Name{name}, _Stage{Uninitial}, _Level{0}, _PlanId{-1}
{}

Rule::Rule():
  Level{
    [&]() -> UInt&{ return *((UInt*)(&_Level)); },
    [&](UInt value){
      if (IsReady()) {
        throw Except(EBadAccess, "change rule\'s level when it\'s ready");
      } else {
        _Level = value;
      }
    }
  },
  Name{
    [&]() -> String&{ return _Name; },
    [&](String value){
      if (_Name.size() > 0) {
        throw Except(ENoSupport, "Don\'t support rename rule");
      } else {
        _Name = value;
      }
    }
  },
  _Name{}, _Stage{Uninitial}, _Level{0}, _PlanId{-1}
{}

Rule::~Rule() { Done(None); }

Bool Rule::Done(Rule* dependency) {
  /* @NOTE: dependency will indicate how the rule would do if it call with
   *  existing dependency, we will remove this out of vector _Up. Otherwide
   *  we will notify to its decendants that this rule has been done */

  if (dependency && dependency != this) {
    /* @NOTE: check and remove it out of _Up list */

    for (UInt i = 0; i < _Up.size(); ++i){
      if (_Up[i].get() == dependency) {
        _Up.erase(_Up.begin() + i);
        return True;
      }
    }
    return False;
  } else {
    /* @NOTE: notify to its decendants that it will be depart soon */

    if (_Up.size() != 0) {
      return False;
    } else {
      for (auto& node: _Down) {
        if (!node->Done(this)) {
          return False;
        }
      }
      return True;
    }
  }
}

Bool Rule::DependBy(Shared<Rule>&& rule) {
  if (Find(_Up.begin(), _Up.end(), rule) < 0) {
    return !BadLogic("duplicate dependency");
  } else if (rule->WaitBy(shared_from_this())) {
    _Up.push_back(rule);
    return True;
  } else {
    return False;
  }
}

Bool Rule::DependBy(Shared<Rule>& rule) {
  return DependBy(std::move(rule));
}

Bool Rule::WaitBy(Shared<Rule>&& rule) {
  if (Find(_Down.begin(), _Down.end(), rule) < 0) {
    return !BadLogic("duplicate dependency");
  } else {
    _Down.push_back(rule);
    return True;
  }
}

Bool Rule::IsReady() { return _Up.size() == 0; }

Bool Rule::Switch(Graph& UNUSED(graph)) { return True; }
}  // namespace Base
