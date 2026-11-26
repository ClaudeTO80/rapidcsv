// test118.cpp - in-place cell finishing (trim and unquote) and UTF-16 parsing without copies

#include <rapidcsv.h>
#include "unittest.h"

#include <stdexcept>
#include <string>

// Encodes an ASCII string as UTF-16LE with a byte order mark.
static std::string ToUtf16LeWithBom(const std::string& pAscii)
{
  std::string out;
  out += '\xff';
  out += '\xfe';
  for (const char ch : pAscii)
  {
    out += ch;
    out += '\0';
  }
  return out;
}

int main()
{
  int rv = 0;

  try
  {
    // With trimming, surrounding blanks are removed before the quotes are checked.
    {
      std::istringstream input("a,b\n  \"x\"\"y\"  ,2\n");
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1),
                             rapidcsv::SeparatorParams(',', true));
      unittest::ExpectEqual(std::string, doc.GetCell<std::string>(0, 0), std::string("x\"y"));
    }

    // Without trimming, a leading blank keeps the quotes from being recognised.
    {
      std::istringstream input("a,b\n  \"x\"\"y\"  ,2\n");
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1),
                             rapidcsv::SeparatorParams(',', false));
      unittest::ExpectEqual(std::string, doc.GetCell<std::string>(0, 0), std::string("  \"x\"\"y\"  "));
    }

    // Auto-quote disabled keeps the quote characters.
    {
      std::istringstream input("a,b\n\"x\",2\n");
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1),
                             rapidcsv::SeparatorParams(',', false, false, false, false));
      unittest::ExpectEqual(std::string, doc.GetCell<std::string>(0, 0), std::string("\"x\""));
    }

    // A lone quote opens a quoted field that is never closed: rejected, not split.
    {
      bool thrown = false;
      try
      {
        std::istringstream input("a,b\n\",2\n");
        rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1));
      }
      catch (const std::runtime_error&)
      {
        thrown = true;
      }
      unittest::ExpectTrue(thrown);
    }

    // UTF-16LE input parses to the same cells as its UTF-8 equivalent.
    {
      const std::string utf8 = "a,b\n\"x\",2\n3,4\n";
      std::istringstream input(ToUtf16LeWithBom(utf8));
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1));
      unittest::ExpectEqual(std::string, doc.GetCell<std::string>(0, 0), std::string("x"));
      unittest::ExpectEqual(int, doc.GetCell<int>(1, 1), 4);
      unittest::ExpectEqual(size_t, doc.GetRowCount(), static_cast<size_t>(2));
    }
  }
  catch (const std::exception& ex)
  {
    std::cout << "exception: " << ex.what() << std::endl;
    rv = 1;
  }

  return rv;
}
