// test116.cpp - float and double parsing: subnormal, nan/inf, whitespace, '+' and partial matches

#include <rapidcsv.h>
#include "unittest.h"

#include <cmath>
#include <limits>
#include <stdexcept>

// Reads the single cell of a one-row document with the default converter.
template <typename T>
static T ReadCell(const std::string& pCell, const rapidcsv::ConverterParams& pParams = rapidcsv::ConverterParams())
{
  std::istringstream input(pCell + "\n");
  rapidcsv::Document doc(input, rapidcsv::LabelParams(-1, -1), rapidcsv::SeparatorParams(), pParams);
  return doc.GetCell<T>(0, 0);
}

int main()
{
  int rv = 0;

  try
  {
    // Subnormal values must be read back, not rejected as out of range.
    {
      const double v = ReadCell<double>("1e-310");
      unittest::ExpectTrue(v == 1e-310);
      const double d = ReadCell<double>("4.9406564584124654e-324");
      unittest::ExpectTrue(d == std::numeric_limits<double>::denorm_min());
      const float f = ReadCell<float>("1.40129846e-45");
      unittest::ExpectTrue(f == std::numeric_limits<float>::denorm_min());
    }

    // nan and inf are accepted with the default converter.
    {
      unittest::ExpectTrue(std::isnan(ReadCell<double>("nan")));
      unittest::ExpectTrue(ReadCell<double>("inf") == std::numeric_limits<double>::infinity());
      unittest::ExpectTrue(ReadCell<double>("-inf") == -std::numeric_limits<double>::infinity());
    }

    // Surrounding whitespace and a leading '+' are accepted.
    {
      unittest::ExpectTrue(ReadCell<double>("  2.5  ") == 2.5);
      unittest::ExpectTrue(ReadCell<double>("+7.25") == 7.25);
    }

#if defined(__cplusplus) && __cplusplus >= 201703L
    // A partially consumed number is an error, not its prefix.
    {
      bool thrown = false;
      try
      {
        ReadCell<double>("\"3,5\"");
      }
      catch (const std::invalid_argument&)
      {
        thrown = true;
      }
      unittest::ExpectTrue(thrown);
    }

    // With a default converter the same input yields the default value.
    {
      const rapidcsv::ConverterParams params(true);
      unittest::ExpectTrue(std::isnan(ReadCell<double>("\"3,5\"", params)));
    }
#endif
  }
  catch (const std::exception& ex)
  {
    std::cout << "exception: " << ex.what() << std::endl;
    rv = 1;
  }

  return rv;
}
