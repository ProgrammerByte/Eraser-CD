#include <clang-c/Index.h>
#include <iostream>

using namespace std;

ostream &operator<<(ostream &stream, const CXString &str) {
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

int main() {
  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit = clang_parseTranslationUnit(
      index, "../test_files/single_files/sum_nums.c", nullptr, 0, nullptr, 0,
      CXTranslationUnit_None);
  if (unit == nullptr) {
    cerr << "Unable to parse translation unit. Quitting." << endl;
    exit(-1);
  }

  CXCursor cursor = clang_getTranslationUnitCursor(unit);
  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData client_data) {
        if (clang_Location_isFromMainFile(clang_getCursorLocation(c)) == 0) {
          return CXChildVisit_Continue;
        }
        cout << "Cursor '" << clang_getCursorSpelling(c) << "' of kind '"
             << clang_getCursorKindSpelling(clang_getCursorKind(c)) << " foo"
             << "'\n";
        return CXChildVisit_Recurse;
      },
      nullptr);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}