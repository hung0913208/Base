#include <Graph.h>
#include <Vertex.h>

namespace Base{
Graph::~Graph() { }

Vector<Weak<Rule>>& Graph::Ready() { return _Ready; }

Bool Graph::IsExist(Shared<Rule>&& rule) {
  for (auto& item: _Rules) {
    if (std::get<1>(item) == rule) {
      return True;
    }
  }

  return False;
}

Bool Graph::IsExist(Shared<Rule>& rule) {
  return IsExist(std::move(rule));
}

Bool Graph::Assign(Shared<Rule>&& rule, Vector<String>&& dependencies) {
  Vertex<void> escaping{[&](){ Lock(); }, [&](){ Unlock(); }};
  UInt count_on_planing{0};

  if (_Rules.find(rule->Name()) != _Rules.end()) {
    if (*Stage(*this, _Rules[rule->Name()]) != Rule::Uninitial) {
      return !(BadLogic << "duplicate " << rule->Name());
    }
  }

  /* @NOTE: check status of rule's dependencies */
  for (auto& depend_by: dependencies) {
    /* @NOTE: sometime, dependency will be apply before its definition
     * so we build an anonymous rule */

    if (depend_by == rule->Name()) {
      return !(BadLogic << "please don\'t define itself as dependency");
    } else if (_Rules.find(depend_by) == _Rules.end()) {
      _Rules[depend_by] = std::make_shared<Rule>();
    } else if ((*Stage(*this, _Rules[depend_by])) == Rule::Finished) {
      continue;
    }

    count_on_planing++;
  }

  /* @NOTE: eveything okey, save this rule */
  _Rules[rule->Name()] = rule;

  /* @NOTE: check and add rule to Queue ready */
  if (!count_on_planing) {
    _Ready.push_back(Weak<Rule>{_Rules[rule->Name()]});
  }
  return True;
}

Bool Graph::Assign(Shared<Rule>&& rule) {
  return Assign(RValue(rule), Vector<String>{});
}

Bool Graph::Assign(Shared<Rule>& rule, Vector<String>&& dependencies){
  return Assign(std::move(rule), RValue(dependencies));
}

Bool Graph::Assign(Shared<Rule>& rule){
  return Assign(std::move(rule));
}

Bool Graph::Done(Weak<Rule> pointer) {
  Vertex<void> escaping{[&](){ Lock(); }, [&](){ Unlock(); }};
  Shared<Rule> rule = pointer.lock();

  /* @NOTE: check if the rule has been removed by another threads or not */
  if (!rule) return False;

  /* @NOTE: remove this rule now, at the time when rule has been decline out
   * of Graph, it will update automatically the graph status */
  if (rule->Done()) {
    _Rules.erase(rule->Name());
  }
  return True;
}

void Graph::Lock() { _Lock(); }

void Graph::Unlock(Bool all) {
  if (all) {
    /* @NOTE: unlock existence rules */

    for (auto& item: _Rules) {
      std::get<1>(item)->Lock() = False;
    }
  }
  _Lock();
}

Int* Graph::Stage(Graph& graph, Shared<Rule> rule) {
  if (!graph.IsExist(RValue(rule))) {
    return None;
  } else {
    return (Int*)&(rule->_Stage);
  }
}

Int* Graph::PlanId(Graph& graph, Shared<Rule> rule) {
  if (!graph.IsExist(rule)) {
    return None;
  } else {
    return &(rule->_PlanId);
  }
}

Vector<Shared<Rule>>* Graph::Up(Graph& graph, Shared<Rule> rule) {
  if (!graph.IsExist(rule)) {
    return None;
  } else {
    return &(rule->_Up);
  }
}

Vector<Shared<Rule>>* Graph::Down(Graph& graph, Shared<Rule> rule) {
  if (!graph.IsExist(rule)) {
    return None;
  } else {
    return &(rule->_Down);
  }
}

Bool Graph::IsInf(Graph& graph, Shared<Rule>& rule) {
#if USE_FINITE_GRAPH_CHECK
  UInt current_possition = 0, current_margin = 1, next_margin = 1, layer = 0;
#endif
  Vertex<void> escaping{[&](){ graph.Lock(); },
                        [&](){ graph.Unlock(True); }};
  Queue<Shared<Rule>> checking{};

  /* @NOTE: lock the first node before doing anything */
  rule->Lock = True;
  checking.push(rule);

  /* @NOTE: check each possible flow from this rule to make sure no loop
   * from decendants to it */
  do {
    auto flow = Down(graph, checking.front());

    /* @NOTE: check if rule is on ready, so we don't need to do anymore */
    if (!flow) {
      return True;
    } else if (rule->Level() == 0) {
      return !rule->IsReady();
    }

    /* @NOTE: check and push node to queue to wait checking */
    for (auto& node: *flow) {
      if (!node->Lock()) {
        return True;
      } else {
        node->Lock = True;

#if USE_FINITE_GRAPH_CHECK
        next_margin++;

        if (layer < graph.MaxCheck)
#endif
        {
          checking.push(node);
        }
      }
    }

    /* @NOTE: done this node, pop out and recheck with new node */
    checking.pop();

#if USE_FINITE_GRAPH_CHECK
    /* @NOTE: use margin to check where we reach the next layer since, node will
     *  be gather according top-down */

    current_possition++;
    if (current_possition >= current_margin) {
      current_margin = next_margin;
      layer++;
    }
  } while (!checking.empty() && layer < graph.MaxCheck);
#else
  } while (!checking.empty());
#endif

  return False;
}
} // namespace Base
