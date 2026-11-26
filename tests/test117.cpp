// test117.cpp - unterminated quoted fields are rejected; out-of-range cell errors name their indices

#include <rapidcsv.h>
#include "unittest.h"

#include <stdexcept>
#include <string>

// Returns true if loading pInput throws std::runtime_error.
static bool LoadThrows(const std::string& pInput)
{
  try
  {
    std::istringstream input(pInput);
    rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1));
    return false;
  }
  catch (const std::runtime_error&)
  {
    return true;
  }
}

// Returns the message of the std::out_of_range thrown by pAccess, or an empty string.
template <typename F>
static std::string OutOfRangeMessage(F pAccess)
{
  try
  {
    pAccess();
  }
  catch (const std::out_of_range& ex)
  {
    return ex.what();
  }
  return std::string();
}

int main()
{
  int rv = 0;

  try
  {
    // A quote that is never closed must not swallow the rest of the input.
    unittest::ExpectTrue(LoadThrows("a,b\n\"unterminated,2\n3,4\n"));
    unittest::ExpectTrue(LoadThrows("a,b\n1,\"open"));

    // A quoted field closed at end of input is valid.
    unittest::ExpectTrue(!LoadThrows("a,b\n1,\"closed\""));
    unittest::ExpectTrue(!LoadThrows("a,b\n1,\"\""));

    // Escaped quotes inside a quoted field do not close it.
    unittest::ExpectTrue(!LoadThrows("a,b\n1,\"x\"\"y\"\n"));

    // A row shorter than the header reports the column and row indices.
    {
      std::istringstream input("a,b,c\n1,2\n");
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1));
      const std::string msg = OutOfRangeMessage([&] { (void)doc.GetCell<std::string>(2, 0); });
      unittest::ExpectEqual(std::string, msg,
                            std::string("column index 2 out of range for row 0 (row has 2 cells)"));
    }

    // A row index beyond the last row reports the row count.
    {
      std::istringstream input("a,b\n1,2\n");
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1));
      const std::string msg = OutOfRangeMessage([&] { (void)doc.GetCell<std::string>(0, 5); });
      unittest::ExpectEqual(std::string, msg, std::string("row index 5 out of range (rows: 1)"));
    }

    // A valid cell is still returned by the function-converter overload.
    {
      std::istringstream input("a,b\n1,2\n");
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, -1));
      const int v = doc.GetCell<int>(1, 0, [](const std::string& s, int& out) { out = std::stoi(s); });
      unittest::ExpectEqual(int, v, 2);
    }
  }
  catch (const std::exception& ex)
  {
    std::cout << "exception: " << ex.what() << std::endl;
    rv = 1;
  }

  return rv;
}
