#include "gui/sm/view_type.hpp"
#include "util/rapidjson/document.h"
#include "util/rapidjson/filereadstream.h"
#include "util/sparse_matrix.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <vector>

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
                           const std::string& coord_type) {
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
      return "v::Chain<v::ListBase<" + coord_type + ", 2>, " +
          std::to_string(ToChainDim(val["direction"])) + ">";
    default:
      throw std::invalid_argument("not a nesting type");
  }
}

typedef SparseMatrix<std::string> SM;

class Indexes {
 public:
  Indexes(unsigned row, unsigned col) : row(row), col(col) {}
  unsigned row, col;
};

const char* kIndent = "  ";
const char kLF = '\n';

void Indent(std::string* indent) { (*indent).append(kIndent); }
void Unindent(std::string* indent) {
  indent->resize(indent->size() - strlen(kIndent));
}

Indexes Generate(const JsonValue& val, const SM& sm,
                 const std::string& coord_type, std::string* indent,
                 std::ostream* os) {
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
          "v::ListBase<" << coord_type << ", 2>" <<  // TODO: find common type
          " >()" << kLF;
      break;
    case kViewTypeDiag:
      *os << "[] {" << kLF
          << *indent << "v::Diag<" << coord_type << ", unsigned" <<
          ", " << val["block_rows"].GetUint() <<
          ", " << val["block_cols"].GetUint() <<
          "> v(ZeroPtr<" << coord_type << ">()" <<
          ", " << size.row << ", " << size.col << ");" << kLF;
      for (auto row = first.row; row <= last.row; ++row) {
        for (auto cols = sm.nonzero_col_range(row, first.col, last.col + 1);
             cols.first != cols.second; ++cols.first)
          *os << *indent << "v(" << row - first.row << ", " <<
              *cols.first - first.col << ") = " << sm(row, *cols.first) <<
              ";" << kLF;
      }
      *os << *indent << "return v;" << kLF;
      Unindent(indent);
      *os << *indent << "}()";
      Indent(indent);
      return first;
    default:
      throw std::runtime_error("unsupported view type");
  }

  std::vector<Indexes> nesting_offsets;
  SMV_JSON_FOR(m, val) {
    const char* name = m->name.GetString();
    if (name[0] == '_') {
      *os << *indent << ".Append(";
      Indent(indent), Indent(indent);
      auto nested_first = Generate(m->value, sm, coord_type, indent, os);
      Unindent(indent), Unindent(indent);
      *os << ")" << kLF;
      nesting_offsets.emplace_back(nested_first.row - first.row,
                                   nested_first.col - first.col);
    }
  }

  switch (type) {
    case kViewTypeChain:
      *os << *indent << ", ZeroPtr<" << coord_type <<
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
              const SM& sm, const std::string& coord_type, std::ostream* os) {
  using namespace std;
  string guard = "CPPVIEWS_BENCH_SM_" + name + "_HPP_";
  transform(guard.begin(), guard.end(), guard.begin(), ::toupper);

  *os << "#ifndef " << guard  << kLF
      << "#define " << guard << kLF
      << kLF
      << R"(#include "../smv_factory.hpp")" << kLF
      << R"(#include "../gui/sm/view_type.hpp")" << kLF
      << kLF
      << R"(#include "../../src/chain.hpp")" << kLF
      << R"(#include "../../src/diag.hpp")" << kLF
      << kLF
      << "class " << name << kLF
      << R"(#define SM_BASE_TYPE \)" << kLF
      << kIndent << GetNestingType(doc, coord_type) <<
      "  // avoid type repetition" << kLF
      << kIndent << kIndent << ": public SM_BASE_TYPE {" << kLF
      << kIndent << "typedef SM_BASE_TYPE BaseType;" << kLF
      << "#undef SM_BASE_TYPE" << kLF
      << kLF
      << string(strlen(kIndent) / 2, ' ') << "public:" << kLF
      << kIndent << name << "() : BaseType(";

  string indent(kIndent);
  Indent(&indent), Indent(&indent);
  Generate(doc, sm, coord_type, &indent, os);
  Unindent(&indent), Unindent(&indent);

  *os << ") {" << kLF
      << kIndent << "}" << kLF
      << "};" << kLF
      << kLF
      << "#endif  // " << guard << kLF;
}

}  // namespace

int main(int argc, char* argv[]) {
  using namespace std;
  cout.sync_with_stdio(false);

  if (argc <= 1) {
    cerr << "Usage: " << argv[0] << " SMVD_FILE [COORD_TYPE]" << endl;
    return 1;
  }
  const string json_path(argv[1]);
  const string coord_type(argc > 2 ? argv[2] : "int"); // TODO: parse from mtx

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
    ifstream mtx_stream((basename_path + ".mtx").c_str());
    if (!mtx_stream) {
      cerr << "Error opening SMVD file: " << strerror(errno) << endl;
      return errno;
    }
    sm.Init(mtx_stream);
  }

  Generate(name, doc, sm, coord_type, &cout);
  return 0;
}
