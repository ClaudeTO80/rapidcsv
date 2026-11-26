// test119.cpp - StorageMode: read-only documents reject every change and read like editable ones

#include <rapidcsv.h>
#include "unittest.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::string Save(rapidcsv::Document& pDoc)
{
  std::ostringstream output;
  pDoc.Save(output);
  return output.str();
}

// Returns true if pChange throws std::logic_error.
template <typename F>
static bool ThrowsLogicError(F pChange)
{
  try
  {
    pChange();
  }
  catch (const std::logic_error&)
  {
    return true;
  }
  return false;
}

int main()
{
  int rv = 0;

  try
  {
    const std::string csv = "id,name,value\nr1,\"a, b\",10\nr2,\"x\"\"y\",20\nr3,plain,30\n";

    // Read-only documents reject each kind of change, and keep their data.
    {
      std::istringstream input(csv);
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(),
                             rapidcsv::ConverterParams(), rapidcsv::LineReaderParams(),
                             rapidcsv::StorageMode::ReadOnly);
      const std::string before = Save(doc);

      unittest::ExpectTrue(ThrowsLogicError([&] { doc.SetCell<int>(0, 0, 1); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.SetCell<int>("value", "r1", 1); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.SetRow<int>(0, std::vector<int>{1, 2}); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.SetColumn<int>(1, std::vector<int>{1, 2}); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.RemoveRow(0); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.RemoveColumn(1); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.InsertRow<int>(1); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.InsertColumn<int>(1); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.SetColumnName(1, "renamed"); }));
      unittest::ExpectTrue(ThrowsLogicError([&] { doc.SetRowName(1, "renamed"); }));

      unittest::ExpectEqual(std::string, Save(doc), before);
      unittest::ExpectTrue(doc.GetRowCount() == 3);
    }

    // Editable documents accept the same changes.
    {
      std::istringstream input(csv);
      rapidcsv::Document doc(input, rapidcsv::LabelParams(0, 0));
      doc.SetCell<int>("value", "r1", 99);
      unittest::ExpectEqual(int, doc.GetCell<int>("value", "r1"), 99);
      unittest::ExpectEqual(int, doc.GetCell<int>("value", "r2"), 20);
    }

    // Read-only and editable documents read the same cells, names and output.
    {
      const std::string sample =
        "# comment line\n"
        "id,name,value\n"
        "\n"
        "r1,\"multi\nline\",10\n"
        "r2,\"x\"\"y\",20\n";
      rapidcsv::SeparatorParams sep(',', false, false, true);
      rapidcsv::LineReaderParams lines(true, '#', true);

      std::istringstream inA(sample);
      rapidcsv::Document editable(inA, rapidcsv::LabelParams(0, 0), sep, rapidcsv::ConverterParams(),
                                  lines, rapidcsv::StorageMode::Editable);
      std::istringstream inB(sample);
      rapidcsv::Document readOnly(inB, rapidcsv::LabelParams(0, 0), sep, rapidcsv::ConverterParams(),
                                  lines, rapidcsv::StorageMode::ReadOnly);

      unittest::ExpectTrue(editable.GetRowCount() == readOnly.GetRowCount());
      unittest::ExpectTrue(editable.GetColumnCount() == readOnly.GetColumnCount());
      unittest::ExpectTrue(editable.GetRowNames() == readOnly.GetRowNames());
      unittest::ExpectTrue(editable.GetColumnNames() == readOnly.GetColumnNames());
      unittest::ExpectTrue(editable.GetCell<std::string>(0, 0) == readOnly.GetCell<std::string>(0, 0));
      unittest::ExpectEqual(std::string, readOnly.GetCell<std::string>(0, 0), std::string("multi\nline"));
      unittest::ExpectEqual(std::string, Save(readOnly), Save(editable));
    }
  }
  catch (const std::exception& ex)
  {
    std::cout << "exception: " << ex.what() << std::endl;
    rv = 1;
  }

  return rv;
}
