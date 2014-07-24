// The default SM type to be benchmarked is provided via smv_run.hpp
// but it can be overriden through compiler flags (see smv_run.hpp)
#include "smv_run.hpp"

#include "util/program_options.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>

typedef SmvFactory<SM_NAME> SmvFactoryType;
typedef SmvFactoryType::ListType Smv;
typedef SmvFactoryType::CoordType Coord;

#define SMV_NANOS_BETWEEN(before, after)                                \
  std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count()

// Uncomment to effectively eliminate overhead due to constructs like
//      if (!gPO.dry_run()) { ... } else { ... }
// that might cause significant slowdown (due to branch misprediction),
// especially when used in an innermost loop
// #define SMV_DRY_RUN false
// #define SMV_DRY_RUN true

namespace {

struct SmvRunOptions : public ProgramOptions<> {
  SmvRunOptions()
      : list('l', "list", "Show the available benchmarks", this),
#ifndef SMV_DRY_RUN
        dry_run('n', "dry-run",
                "Do not put the actual benchmark workload\n"
                "(useful measuring benchmark overhead)", this),
#endif  // !defined(SMV_DRY_RUN)
        iter_count('i', "iter-count",
                   "Sets the number of times to run the main benchmark loop",
                   this),
        repeat_count('r', "repeat-count",
                     "Sets the number of times to repeat the benchmark\n"
                     "(initialization precedes each run but seed is not reset)",
                     this),
        all_runs('a', "all-runs", "Print times of each benchmark run", this),
        seed('s', "seed", "Sets a positive integer to be used as a seed", this),
        access_count('c', "access-count",
                     "Sets the total number of accesses the sparse matrix view",
                     this) {}

  void PrintUsage(const char* argv0, std::ostream* os) const override {
    *os << "Usage: " << argv0 << " <bench>" << std::endl;
    ProgramOptions<>::PrintUsage(argv0, os);
  }

  const std::string& bench_name() const { return bench_name_; }

  Option<> list;

#ifndef SMV_DRY_RUN
  Option<> dry_run;
#else
  bool dry_run() const { return SMV_DRY_RUN; }
#endif  // !defined(SMV_DRY_RUN)

  Option<unsigned> iter_count;
  Option<unsigned> repeat_count;
  Option<> all_runs;
  Option<unsigned> seed;

  Option<size_t> access_count;

 protected:
  void InitDefaults(int argc, char** argv) {
    SetIfNot(false, &list);
    if (help() || list())
      return;

#ifndef SMV_DRY_RUN
    SetIfNot(false, &dry_run);
#endif  // !defined(SMV_DRY_RUN)

    if (argc <= 1)
      throw Exception("Missing benchmark name");
    bench_name_ = argv[1];
    SetIfNot(1, &iter_count);
    SetIfNot(1, &repeat_count);
    SetIfNot(false, &all_runs);
  }

 private:
  std::string bench_name_;
} gPO;

template<typename T>
struct TimeEntryHelper {
  TimeEntryHelper(std::string&& name, const T& ns)
    : name_(std::move(name)), ns_(ns) {}
  friend std::ostream& operator<<(std::ostream& os, const TimeEntryHelper& t) {
    os << t.name_ <<
        '\t' << std::fixed << std::setprecision(3) << 1e-9 * t.ns_ <<
        '\t' << t.ns_ << "\n";
    return os;
  }
 private:
  std::string name_;
  T ns_;
};

template<typename T>
TimeEntryHelper<T> TimeEntry(std::string&& name, const T& ns) {
  return TimeEntryHelper<T>(std::move(name), ns);
}

struct Benchmark {
  Benchmark() : smv_(SmvFactoryType::Get()) {
    if (gPO.seed.count()) gen_.seed(gPO.seed());
    else gen_.seed(std::random_device{}());
  }
  virtual ~Benchmark() {
    // prevent optimizations by a conditional print that depends on hash_
    if (hash_ == 0xCAFEBABE)
      std::cerr << "Unexpected benchmark hash" << std::endl;
    if (gPO.verbosity())
      std::cerr << "Computed benchmark hash: " << hash_ << std::endl;
  }
  virtual void Init() = 0;
  virtual void Run() = 0;
 protected:
  size_t hash_ = 1;  // used to prevent optimizations
  std::mt19937 gen_;
  const Smv& smv_;
};

struct PermutedRandomAccess : public Benchmark {
  PermutedRandomAccess() {
    if (gPO.access_count.count() && gPO.access_count() < smv_.size())
      std::cerr << "Warning: Not all elements are going to be accessed"
                << std::endl;
  }
  void Init() override {
    const size_t access_count = gPO.access_count.count()
        ? gPO.access_count() : smv_.size();

    coord_pairs_.clear();
    coord_pairs_.reserve(access_count);
    while (coord_pairs_.size() < access_count)
      for (Coord row = RowCount(smv_); row--; )
        for (Coord col = ColCount(smv_); col--; )
          coord_pairs_.emplace_back(row, col);
    std::shuffle(coord_pairs_.begin(), coord_pairs_.end(), gen_);
  }

  void Run() override {
    for (const auto& cp : coord_pairs_) {
      if (!gPO.dry_run())
        hash_ += smv_(cp.first, cp.second);
      else
        hash_ += cp.first + cp.second;
    }
  }

 private:
  typedef std::pair<Coord, Coord> CoordPair;
  std::vector<CoordPair> coord_pairs_;
};

}  // namespace

int main(int argc, char* argv[]) {
  using namespace std;
  try {
    gPO.Parse(&argc, argv);
    if (gPO.help()) {
      gPO.PrintUsage(argv[0], &cout);
      return 0;
    }
  } catch (const ProgramOptions<>::Exception& e) {
    cerr << e.what() << endl;
    gPO.PrintUsage(argv[0], &cerr);
    return 1;
  }

  map<string, function<Benchmark*()> > bench_creators = {
    {"pra", [] { return new PermutedRandomAccess; } },
  };

  if (gPO.list()) {
    for (const auto& e : bench_creators)
      cerr << e.first << "\n";
    return 0;
  }

  unique_ptr<Benchmark> bench;
  using namespace chrono;
  try {
    auto bench_creator = bench_creators.at(gPO.bench_name());
    if (gPO.verbosity())
      cerr << "Constructing benchmark " << gPO.bench_name() << " ..." << endl;

    auto real_before = steady_clock::now();
    bench.reset(bench_creator());
    auto real_after = steady_clock::now();

    if (gPO.verbosity())
      cerr << "Construction took "
           << SMV_NANOS_BETWEEN(real_before, real_after) << " ns" << endl;
  } catch (const out_of_range&) {
    cerr << "Unsupported benchmark: " << gPO.bench_name() << endl;
    return 1;
  }


  const auto kRealMin = []{
    auto real_before = steady_clock::now();
    auto real_after = steady_clock::now();
    return SMV_NANOS_BETWEEN(real_before, real_after);
  }();
  if (gPO.verbosity())
    cerr << "Timing overhead is " << kRealMin << " ns" << endl;

  unsigned long long real_total = 0;  // total real time in ns
  for (unsigned run = 0; run < gPO.repeat_count(); ++run) {
    if (gPO.verbosity() > 1) {
      cerr << "Initializing benchmark " << gPO.bench_name() << " ..." << endl;
      auto real_before = steady_clock::now();
      bench->Init();
      auto real_after = steady_clock::now();
      cerr << "Initialization took "
           << SMV_NANOS_BETWEEN(real_before, real_after) << " ns\n"
           << "Running benchmark " << gPO.bench_name() << " ..." << endl;
    } else
      bench->Init();

    {
      auto real_before = steady_clock::now();
      for (unsigned iter = gPO.iter_count(); iter--; )
        bench->Run();
      auto real_after = steady_clock::now();
      auto real_time = SMV_NANOS_BETWEEN(real_before, real_after);
      if (real_time <= kRealMin) {
        cerr << "Error: Benchmark is too short" << endl;
        return 13;
      }
      real_total += real_time;

      if (gPO.all_runs()) {
        cout << TimeEntry("run#" + to_string(run), real_time);
      }
    }
  }
  auto real_avg = real_total / gPO.repeat_count();
  cout << TimeEntry("avg", real_avg) << TimeEntry("total", real_total);

  return 0;
}