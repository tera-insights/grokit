// This file includes commonly used components of GISTs, such as various cGLAs,
// LocalSchedulers, and Tasks.

// This cGLA always return false, meaning whatever tasks are not repeated.
class HaltingGLA {
 public:
  HaltingGLA() {}
  void AddState(HaltingGLA other) {}
  bool ShouldIterate() { return false; }
};

// This cGLA returns whichever value it is initialized with. All instances of
// this cGLA in the same iteration should be initialized with the same value.
class AnswerGLA {
  public:
    AnswerGLA(bool answer) : answer_(answer) {}
    void AddState(AnswerGLA other) {}
    bool ShouldIterate() { return answer_; }
  private:
    bool answer_;
};

// This scheduler is templated with Task type T and returns a single Task using
// the value it is initialized with.
template<class T>
class SimpleScheduler {
 private:
  // The task outputted by this scheduler.
  T task_;

  // Whether the singular Task has been scheduled.
  bool finished;

 public:
  SimpleScheduler(T task) : task_(task), finished(false) {};

  bool GetNextTask(T& task) {
    task = task_;
    return finished = !finished;
  }
};

// This scheduler is templated Task type T and is initialized with an upper and
// lower bound. It returns one task for each value in the interval [begin, end).
// T should support a pre-increment operator and an equality comparison. It can
// be used with iterators instead of simple integer indices, but the object that
// is being iterated over must have a lifespan of at least a round for the GIST.
template<class T>
class BlockScheduler {
 private:
  // The current and last (exclusive) task to be outputted by this scheduler.
  T current_, end_;

 public:
  BlockScheduler(T begin, T end) : current_(begin), end_(end) {};

  bool GetNextTask(T& task) {
    if (current_ == end_)
      return false;
    task = current_;
    ++current;
  }
}
