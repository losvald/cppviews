#include "gui/sm/view_type.hpp"
#include "util/rapidjson/document.h"
#include "util/rapidjson/filereadstream.h"
#include "util/sparse_matrix.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <vector>

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <cstring>

#define SMV_JSON_FOR(m, val) \
  for (auto m = val.MemberBegin(), m_end = val.MemberEnd(); m != m_end; ++m)

namespace {

typedef rapidjson::GenericValue<rapidjson::Document::EncodingType,
                                rapidjson::Document::AllocatorType> JsonValue;

unsigned ToChainDim(const JsonValue& direction_val) {
  return 1 - direction_val.GetUint();
}

std::string GetNestingType(const JsonValue& val,
                           const std::string& data_type) {
  // TODO: determine common type by unification of nested view types
  // SMV_JSON_FOR(m, val) {
  //   const char* name = m->name.GetString();
  //   if (name[0] == '_') {
  //     ...
  //   }
  // }
  ViewType type = static_cast<ViewType>(val["type"].GetInt());
  switch (type) {
    case kViewTypeChain:
      return "v::Chain<v::ListBase<" + data_type + ", 2>, " +
          std::to_string(ToChainDim(val["direction"])) + ">";
    default:
      throw std::invalid_argument("not a nesting type");
  }
}

typedef SparseMatrix<std::string> SM;

struct Indexes {
  Indexes(unsigned row, unsigned col) : row(row), col(col) {}
  unsigned row, col;
};

struct Assignment : public Indexes {
  Assignment(unsigned row, unsigned col, const std::string& val)
      : Indexes(row, col), val(val) {}
  std::string val;
};

const char* kIndent = "  ";
const char kLF = '\n';

void Indent(std::string* indent) { (*indent).append(kIndent); }
void Unindent(std::string* indent) {
  indent->resize(indent->size() - strlen(kIndent));
}

bool AllSame(const SM& sm, const Indexes& first, const Indexes& last,
             std::string* unique_val) {
  unique_val->clear();
  for (auto row = first.row; row <= last.row; ++row) {
    for (auto cols = sm.nonzero_col_range(row, first.col, last.col + 1);
         cols.first != cols.second; ++cols.first) {
      const std::string& val = sm(row, *cols.first);
      if (unique_val->empty()) *unique_val = val;
      else if (*unique_val != val) return false;
    }
  }
  return true;
}

Indexes Generate(const JsonValue& val, const SM& sm,
                 const std::string& data_type, std::vector<Assignment>* asgns,
                 std::string* indent, std::ostream* os) {
  const Indexes first(val["first_row"].GetUint(), val["first_col"].GetUint());
  const Indexes last(val["last_row"].GetUint(), val["last_col"].GetUint());
  const Indexes min(std::min(first.row, last.row),
                    std::min(first.col, last.col));
  const Indexes max(std::max(first.row, last.row),
                    std::max(first.col, last.col));
  const Indexes size((max.row - min.row) + 1, (max.col - min.col) + 1);
  ViewType type = static_cast<ViewType>(val["type"].GetInt());
  switch (type) {
    case kViewTypeChain:
      *os << kLF
          << *indent << "v::ChainTag<" <<
          ToChainDim(val["direction"]) <<
          ">(), v::ListVector<" <<
          "v::ListBase<" << data_type << ", 2>" <<  // TODO: find common type
          " >()" << kLF;
      break;
    case kViewTypeDiag: {
      *os << "[] {" << kLF
          << *indent << "v::Diag<" << data_type << ", unsigned" <<
          ", " << val["block_rows"].GetUint() <<
          ", " << val["block_cols"].GetUint() <<
          "> v(ZeroPtr<" << data_type << ">()" <<
          ", " << size.row << ", " << size.col << ");" << kLF;

      // check if all values are same (to avoid huge compilation time)
      std::string unique_val;
      if (AllSame(sm, first, last, &unique_val)) {
        *os << *indent << "for (unsigned i = 0; i < " << size.row <<
            "; ++i)" << kLF
            << *indent << kIndent << "v(i, i) = " << unique_val << ";" << kLF;
      } else {
        for (auto row = first.row; row <= last.row; ++row) {
          for (auto cols = sm.nonzero_col_range(row, first.col, last.col + 1);
               cols.first != cols.second; ++cols.first)
            asgns->emplace_back(row, *cols.first, sm(row, *cols.first));
        }
      }
      *os << *indent << "return v;" << kLF;
      Unindent(indent);
      *os << *indent << "}()";
      Indent(indent);
      return first;
    }
    default:
      throw std::runtime_error("unsupported view type");
  }

  std::vector<Indexes> nesting_offsets;
  SMV_JSON_FOR(m, val) {
    const char* name = m->name.GetString();
    if (name[0] == '_') {
      *os << *indent << ".Append(";
      Indent(indent), Indent(indent);
      auto nested_first = Generate(m->value, sm, data_type, asgns, indent, os);
      Unindent(indent), Unindent(indent);
      *os << ")" << kLF;
      nesting_offsets.emplace_back(nested_first.row - first.row,
                                   nested_first.col - first.col);
    }
  }

  switch (type) {
    case kViewTypeChain:
      *os << *indent << ", ZeroPtr<" << data_type <<
          ">(), v::ChainOffsetVector<2>({" << kLF;
      Indent(indent), Indent(indent); {
        for (const auto& nesting_offset : nesting_offsets)
          *os << *indent << '{' << nesting_offset.row << ", " <<
              nesting_offset.col << "}," << kLF;
        Indent(indent), Indent(indent);
        *os << *indent << "})" << kLF;
        Unindent(indent), Unindent(indent);
      } Unindent(indent), Unindent(indent);
      *os << *indent << ", " << size.row << ", " << size.col;
      break;
    default:
      break;
  }
  return first;
}

void Generate(const std::string& name, const rapidjson::Document& doc,
              const SM& sm, const std::string& data_type, std::ostream* os) {
  using namespace std;
  string guard = "CPPVIEWS_BENCH_SM_VIEW_" + name + "_HPP_";
  transform(guard.begin(), guard.end(), guard.begin(), ::toupper);

  *os << "#ifndef " << guard  << kLF
      << "#define " << guard << kLF
      << kLF
      << R"(#include "../../smv_factory.hpp")" << kLF
      << R"(#include "../../gui/sm/view_type.hpp")" << kLF
      << kLF
      << R"(#include "../../../src/chain.hpp")" << kLF
      << R"(#include "../../../src/diag.hpp")" << kLF
      << kLF
      << "class " << name << kLF
      << R"(#define SM_BASE_TYPE \)" << kLF
      << kIndent << GetNestingType(doc, data_type) <<
      "  // avoid type repetition" << kLF
      << kIndent << kIndent << ": public SM_BASE_TYPE {" << kLF
      << kIndent << "typedef SM_BASE_TYPE BaseType;" << kLF
      << "#undef SM_BASE_TYPE" << kLF
      << kLF
      << string(strlen(kIndent) / 2, ' ') << "public:" << kLF
      << kIndent << name << "() : BaseType(";

  std::vector<Assignment> asgns;
  string indent(kIndent);
  Indent(&indent), Indent(&indent);
  Generate(doc, sm, data_type, &asgns, &indent, os);
  Unindent(&indent), Unindent(&indent);

  *os << ") {" << kLF;
  Indent(&indent);

  // // For some reason, this results in an internal compiler bug + very slow
  // for (const auto& asgn : asgns) {
  //   *os << indent << "(*this)(" << asgn.row << ", " << asgn.col << ") = " <<
  //       asgn.val << ";" << kLF;
  // }

#define SMV_GEN_STATIC_ARRAY(type, name, asgn_field)                     \
  do {                                                                  \
    *os << indent << "static " << type << " " #name "[] = {" << kLF;    \
        Indent(&indent);                                                \
        for (const auto& asgn : asgns)                                  \
          *os << indent << asgn.asgn_field << ',' << kLF;               \
        Unindent(&indent);                                              \
        *os << indent << "};" << kLF;                                   \
  } while (0)

  SMV_GEN_STATIC_ARRAY(data_type, data, val);
  SMV_GEN_STATIC_ARRAY("unsigned", rows, row);
  SMV_GEN_STATIC_ARRAY("unsigned", cols, col);
#undef SMV_GEN_STATIC_ARRAY

  *os << indent << kLF
      << indent << "for (size_t i = 0; i < " << asgns.size() << "; ++i)" << kLF
      << indent << kIndent << "(*this)(rows[i], cols[i]) = data[i];" << kLF;

  Unindent(&indent);
  *os << indent << "}" << kLF
      << "};" << kLF
      << kLF
      << "#endif  // " << guard << kLF;
}

}  // namespace

int main(int argc, char* argv[]) {
  using namespace std;
  cout.sync_with_stdio(false);

  if (argc <= 2) {
    cerr << "Usage: " << argv[0] << " SMVD_FILE MTX_FILE [DATA_TYPE]" << endl;
    return 1;
  }
  const string json_path(argv[1]);
  const string mtx_path(argv[2]);
  const string data_type(argc > 3 ? argv[3] : "int"); // TODO: parse from mtx

  const string basename_path(json_path.substr(0, json_path.rfind('.')));
  const string name = basename_path.substr(
      basename_path.find_last_of("/\\") + 1);

  rapidjson::Document doc;
  {
    FILE* json_file = fopen(json_path.c_str(), "r");
    if (json_file == nullptr) {
      cerr << "Error opening SMVD file: " << strerror(errno) << endl;
      return errno;
    }
    char buffer[1 << 16];  // 65K buffer
    rapidjson::FileReadStream is(json_file, buffer, sizeof(buffer));
    doc.ParseStream(is);
  }

  SM sm;
  {
    ifstream mtx_stream(mtx_path.c_str());
    if (!mtx_stream) {
      cerr << "Error opening SMVD file: " << strerror(errno) << endl;
      return errno;
    }
    sm.Init(mtx_stream);
  }

  assert(0);
  Generate(name, doc, sm, data_type, &cout);
  return 0;
}
