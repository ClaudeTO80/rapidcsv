/*
 * rapidcsv.h
 *
 * URL:      https://github.com/d99kris/rapidcsv
 * Version:  9.09
 *
 * Copyright (c) 2017-2026 Kristofer Berggren
 * All rights reserved.
 *
 * rapidcsv is distributed under the BSD 3-Clause license, see LICENSE for details.
 *
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

namespace rapidcsv
{
#if defined(_MSC_VER)
  static const bool sPlatformHasCR = true;
#else
  static const bool sPlatformHasCR = false;
#endif
  static const std::vector<char> s_Utf8BOM = { '\xef', '\xbb', '\xbf' };

  /**
   * @brief     Datastructure holding parameters controlling how invalid numbers (including
   *            empty strings) should be handled.
   */
  struct ConverterParams
  {
    /**
     * @brief   Constructor
     * @param   pHasDefaultConverter  specifies if conversion of non-numerical strings shall be
     *                                converted to a default numerical value, instead of causing
     *                                an exception to be thrown (default).
     * @param   pDefaultFloat         floating-point default value to represent invalid numbers.
     * @param   pDefaultInteger       integer default value to represent invalid numbers.
     * @param   pNumericLocale        specifies whether to honor LC_NUMERIC locale (default
     *                                true).
     */
    explicit ConverterParams(const bool pHasDefaultConverter = false,
                             const long double pDefaultFloat = std::numeric_limits<long double>::signaling_NaN(),
                             const long long pDefaultInteger = 0,
                             const bool pNumericLocale = true)
      : mHasDefaultConverter(pHasDefaultConverter)
      , mDefaultFloat(pDefaultFloat)
      , mDefaultInteger(pDefaultInteger)
      , mNumericLocale(pNumericLocale)
    {
    }

    /**
     * @brief   specifies if conversion of non-numerical strings shall be converted to a default
     *          numerical value, instead of causing an exception to be thrown (default).
     */
    bool mHasDefaultConverter;

    /**
     * @brief   floating-point default value to represent invalid numbers.
     */
    long double mDefaultFloat;

    /**
     * @brief   integer default value to represent invalid numbers.
     */
    long long mDefaultInteger;

    /**
     * @brief   specifies whether to honor LC_NUMERIC locale.
     */
    bool mNumericLocale;
  };

  /**
   * @brief     Read-only std::streambuf viewing an existing string without copying it.
   * @details   Used to parse UTF-8 text produced by a UTF-16 conversion. The buffer is only
   *            read through the get area, so the const_cast does not modify the string.
   */
  class StringViewBuf : public std::streambuf
  {
  public:
    explicit StringViewBuf(const std::string& pStr)
    {
      char* begin = const_cast<char*>(pStr.data());
      setg(begin, begin, begin + pStr.size());
    }
  };

  /**
   * @brief     Exception thrown when attempting to access Document data in a datatype which
   *            is not supported by the Converter class.
   */
  class no_converter : public std::exception
  {
  public:
    /**
     * @brief   Provides details about the exception
     * @returns an explanatory string
     */
    const char* what() const throw() override
    {
      return "unsupported conversion datatype";
    }
  };

  /**
   * @brief     Class providing conversion to/from numerical datatypes and strings. Only
   *            intended for rapidcsv internal usage, but exposed externally to allow
   *            specialization for custom datatype conversions.
   */
  template<typename T>
  class Converter
  {
  public:
    /**
     * @brief   Constructor
     * @param   pConverterParams      specifies how conversion of non-numerical values to
     *                                numerical datatype shall be handled.
     */
    Converter(const ConverterParams& pConverterParams)
      : mConverterParams(pConverterParams)
    {
    }

    /**
     * @brief   Converts numerical value to string representation.
     * @param   pVal                  numerical value
     * @param   pStr                  output string
     */
    void ToStr(const T& pVal, std::string& pStr) const
    {
      if (typeid(T) == typeid(int) ||
          typeid(T) == typeid(short) ||
          typeid(T) == typeid(unsigned short) ||
          typeid(T) == typeid(long) ||
          typeid(T) == typeid(long long) ||
          typeid(T) == typeid(unsigned) ||
          typeid(T) == typeid(unsigned long) ||
          typeid(T) == typeid(unsigned long long) ||
          typeid(T) == typeid(long double) ||
          typeid(T) == typeid(char))
      {
        std::ostringstream out;
        out << pVal;
        pStr = out.str();
      }
      else if (typeid(T) == typeid(float))
      {
        std::ostringstream out;
        out << std::setprecision(9) << pVal;
        pStr = out.str();
      }
      else if (typeid(T) == typeid(double))
      {
        std::ostringstream out;
        out << std::setprecision(17) << pVal;
        pStr = out.str();
      }
      else if (typeid(T) == typeid(signed char))
      {
        std::ostringstream out;
        out << static_cast<int>(pVal);
        pStr = out.str();
      }
      else if (typeid(T) == typeid(unsigned char))
      {
        std::ostringstream out;
        out << static_cast<unsigned int>(pVal);
        pStr = out.str();
      }
      else
      {
        throw no_converter();
      }
    }

    /**
     * @brief   Converts string holding a numerical value to numerical datatype representation.
     * @param   pVal                  numerical value
     * @param   pStr                  output string
     */
    void ToVal(const std::string& pStr, T& pVal) const
    {
      try
      {
        if (typeid(T) == typeid(int))
        {
          pVal = static_cast<T>(std::stoi(pStr));
          return;
        }
        else if ((typeid(T) == typeid(signed char)) || (typeid(T) == typeid(unsigned char)) ||
                 (typeid(T) == typeid(short)) || (typeid(T) == typeid(unsigned short)))
        {
          const int i = std::stoi(pStr);
          if ((i < static_cast<int>((std::numeric_limits<T>::min)())) ||
              (i > static_cast<int>((std::numeric_limits<T>::max)())))
          {
            throw std::out_of_range("conversion: out of range");
          }
          pVal = static_cast<T>(i);
          return;
        }
        else if (typeid(T) == typeid(long))
        {
          pVal = static_cast<T>(std::stol(pStr));
          return;
        }
        else if (typeid(T) == typeid(long long))
        {
          pVal = static_cast<T>(std::stoll(pStr));
          return;
        }
        else if ((typeid(T) == typeid(unsigned)) || (typeid(T) == typeid(unsigned long)) ||
                 (typeid(T) == typeid(unsigned long long)))
        {
          const unsigned long long ull = std::stoull(pStr);
          const T val = static_cast<T>(ull);

          // std::stoull() wraps negative values around, instead of throwing
          const size_t signPos = pStr.find_first_not_of(" \f\n\r\t\v");
          const bool isNegative =
            (ull != 0) && (signPos != std::string::npos) && (pStr.at(signPos) == '-');

          if (isNegative || (static_cast<unsigned long long>(val) != ull))
          {
            throw std::out_of_range("conversion: out of range");
          }
          pVal = val;
          return;
        }
      }
      catch (...)
      {
        if (!mConverterParams.mHasDefaultConverter)
        {
          throw;
        }
        else
        {
          pVal = static_cast<T>(mConverterParams.mDefaultInteger);
          return;
        }
      }

      try
      {
        if (mConverterParams.mNumericLocale)
        {
          if (typeid(T) == typeid(float))
          {
            pVal = static_cast<T>(std::stof(pStr));
            return;
          }
          else if (typeid(T) == typeid(double))
          {
            pVal = static_cast<T>(std::stod(pStr));
            return;
          }
          else if (typeid(T) == typeid(long double))
          {
            pVal = static_cast<T>(std::stold(pStr));
            return;
          }
        }
        else
        {
          if ((typeid(T) == typeid(float)) ||
              (typeid(T) == typeid(double)) ||
              (typeid(T) == typeid(long double)))
          {
            std::istringstream iss(pStr);
            iss.imbue(std::locale::classic());
            iss >> pVal;
            if (iss.fail() || iss.bad() || !iss.eof())
            {
              throw std::invalid_argument("istringstream: no conversion");
            }
            return;
          }
        }
      }
      catch (...)
      {
        if (!mConverterParams.mHasDefaultConverter)
        {
          throw;
        }
        else
        {
          pVal = static_cast<T>(mConverterParams.mDefaultFloat);
          return;
        }
      }

      if (typeid(T) == typeid(char))
      {
        pVal = static_cast<T>(pStr[0]);
        return;
      }
      else
      {
        throw no_converter();
      }
    }

  private:
    const ConverterParams& mConverterParams;
  };

  /**
   * @brief     Specialized implementation handling string to string conversion.
   * @param     pVal                  string
   * @param     pStr                  string
   */
  template<>
  inline void Converter<std::string>::ToStr(const std::string& pVal, std::string& pStr) const
  {
    pStr = pVal;
  }

  /**
   * @brief     Specialized implementation handling string to string conversion.
   * @param     pVal                  string
   * @param     pStr                  string
   */
  template<>
  inline void Converter<std::string>::ToVal(const std::string& pStr, std::string& pVal) const
  {
    pVal = pStr;
  }

  template<typename T>
  using ConvFunc = std::function<void (const std::string& pStr, T& pVal)>;

  /**
   * @brief     Datastructure holding parameters controlling which row and column should be
   *            treated as labels.
   */
  struct LabelParams
  {
    /**
     * @brief   Constructor
     * @param   pColumnNameIdx        specifies the zero-based row index of the column labels, setting
     *                                it to -1 prevents column lookup by label name, and gives access
     *                                to all rows as document data. Default: 0
     * @param   pRowNameIdx           specifies the zero-based column index of the row labels, setting
     *                                it to -1 prevents row lookup by label name, and gives access
     *                                to all columns as document data. Default: -1
     */
    explicit LabelParams(const int pColumnNameIdx = 0, const int pRowNameIdx = -1)
      : mColumnNameIdx(pColumnNameIdx)
      , mRowNameIdx(pRowNameIdx)
    {
      if (mColumnNameIdx < -1)
      {
        const std::string errStr = "invalid column name index " +
          std::to_string(mColumnNameIdx) + " < -1";
        throw std::out_of_range(errStr);
      }

      if (mRowNameIdx < -1)
      {
        const std::string errStr = "invalid row name index " +
          std::to_string(mRowNameIdx) + " < -1";
        throw std::out_of_range(errStr);
      }
    }

    /**
     * @brief   specifies the zero-based row index of the column labels.
     */
    int mColumnNameIdx;

    /**
     * @brief   specifies the zero-based column index of the row labels.
     */
    int mRowNameIdx;
  };

  /**
   * @brief     Datastructure holding parameters controlling how the CSV data fields are separated.
   */
  struct SeparatorParams
  {
    /**
     * @brief   Constructor
     * @param   pSeparator            specifies the column separator (default ',').
     * @param   pTrim                 specifies whether to trim leading and trailing spaces from
     *                                cells read (default false).
     * @param   pHasCR                specifies whether a new document (i.e. not an existing document read)
     *                                should use CR/LF instead of only LF (default is to use standard
     *                                behavior of underlying platforms - CR/LF for Win, and LF for others).
     * @param   pQuotedLinebreaks     specifies whether to allow line breaks in quoted text (default false)
     * @param   pAutoQuote            specifies whether to automatically dequote data during read, and add
     *                                quotes during write (default true).
     * @param   pQuoteChar            specifies the quote character (default '\"').
     */
    explicit SeparatorParams(const char pSeparator = ',', const bool pTrim = false,
                             const bool pHasCR = sPlatformHasCR, const bool pQuotedLinebreaks = false,
                             const bool pAutoQuote = true, const char pQuoteChar = '"')
      : mSeparator(pSeparator)
      , mTrim(pTrim)
      , mHasCR(pHasCR)
      , mQuotedLinebreaks(pQuotedLinebreaks)
      , mAutoQuote(pAutoQuote)
      , mQuoteChar(pQuoteChar)
    {
    }

    /**
     * @brief   specifies the column separator.
     */
    char mSeparator;

    /**
     * @brief   specifies whether to trim leading and trailing spaces from cells read.
     */
    bool mTrim;

    /**
     * @brief   specifies whether new documents should use CR/LF instead of LF.
     */
    bool mHasCR;

    /**
     * @brief   specifies whether to allow line breaks in quoted text.
     */
    bool mQuotedLinebreaks;

    /**
     * @brief   specifies whether to automatically dequote cell data.
     */
    bool mAutoQuote;

    /**
     * @brief   specifies the quote character.
     */
    char mQuoteChar;
  };

  /**
   * @brief     Datastructure holding parameters controlling how special line formats should be
   *            treated.
   */
  struct LineReaderParams
  {
    /**
     * @brief   Constructor
     * @param   pSkipCommentLines     specifies whether to skip lines prefixed with
     *                                mCommentPrefix. Default: false
     * @param   pCommentPrefix        specifies which prefix character to indicate a comment
     *                                line. Default: #
     * @param   pSkipEmptyLines       specifies whether to skip empty lines. Default: false
     */
    explicit LineReaderParams(const bool pSkipCommentLines = false,
                              const char pCommentPrefix = '#',
                              const bool pSkipEmptyLines = false)
      : mSkipCommentLines(pSkipCommentLines)
      , mCommentPrefix(pCommentPrefix)
      , mSkipEmptyLines(pSkipEmptyLines)
    {
    }

    /**
     * @brief   specifies whether to skip lines prefixed with mCommentPrefix.
     */
    bool mSkipCommentLines;

    /**
     * @brief   specifies which prefix character to indicate a comment line.
     */
    char mCommentPrefix;

    /**
     * @brief   specifies whether to skip empty lines.
     */
    bool mSkipEmptyLines;
  };

  /**
   * @brief     Selects how the cells of a Document are stored; chosen when data is loaded.
   */
  enum class StorageMode
  {
    Editable,   //!< rows can be inserted, removed and changed (default)
    ReadOnly    //!< cells are stored compactly, and any change throws std::logic_error
  };

  /**
   * @brief     Cell storage of a Document, in one of two representations.
   * @details   Editable: each row keeps its cells in one buffer, with the end offset of each
   *            cell, so single rows can be inserted, removed and rewritten cheaply.
   *            Read-only: all cells share one buffer, with the end offset of each cell and the
   *            cumulative cell count of each row. It uses less memory and reads faster, but
   *            cannot be changed.
   *            Offsets are 32-bit: a row (editable) or the whole table (read-only) is limited
   *            to 4 GiB of cell text.
   */
  class CellTable
  {
  public:
    CellTable()
      : mReadOnly(false)
      , mRows()
      , mArena()
      , mCellEnds()
      , mRowEnds()
      , mPending()
      , mPendingArenaStart(0)
      , mPendingCellStart(0)
    {
    }

    /**
     * @brief   Empties the table and selects its representation.
     * @param   pReadOnly             true for the read-only representation.
     */
    void Reset(const bool pReadOnly)
    {
      mReadOnly = pReadOnly;
      Clear();
    }

    /**
     * @brief   Removes all rows, keeping the representation.
     */
    void Clear()
    {
      mRows.clear();
      mArena.clear();
      mCellEnds.clear();
      mRowEnds.clear();
      mPending.chars.clear();
      mPending.ends.clear();
    }

    bool IsReadOnly() const
    {
      return mReadOnly;
    }

    size_t RowCount() const
    {
      return mReadOnly ? mRowEnds.size() : mRows.size();
    }

    size_t RowWidth(const size_t pRow) const
    {
      CheckRow(pRow);
      if (mReadOnly)
      {
        return mRowEnds[pRow] - RowFirstCell(pRow);
      }
      return mRows[pRow].ends.size();
    }

    /**
     * @brief   Returns the content of one cell. The indices must be valid.
     */
    std::string Get(const size_t pRow, const size_t pCol) const
    {
      CheckCell(pRow, pCol);
      if (mReadOnly)
      {
        const size_t cell = RowFirstCell(pRow) + pCol;
        const size_t begin = (cell == 0) ? 0 : mCellEnds[cell - 1];
        return mArena.substr(begin, mCellEnds[cell] - begin);
      }

      const RowBuf& row = mRows[pRow];
      const size_t begin = (pCol == 0) ? 0 : row.ends[pCol - 1];
      return row.chars.substr(begin, row.ends[pCol] - begin);
    }

    /**
     * @brief   Copies the content of one cell into pOut, reusing its buffer.
     * @details Unlike Get(), performs no bounds check: the indices must be valid. Avoids the
     *          temporary string that Get() returns by value.
     * @param   pRow                  row index, valid
     * @param   pCol                  column index within the row, valid
     * @param   pOut                  destination string, overwritten
     */
    void CopyCell(const size_t pRow, const size_t pCol, std::string& pOut) const
    {
      if (mReadOnly)
      {
        const size_t cell = RowFirstCell(pRow) + pCol;
        const size_t begin = (cell == 0) ? 0 : mCellEnds[cell - 1];
        pOut.assign(mArena, begin, mCellEnds[cell] - begin);
        return;
      }

      const RowBuf& row = mRows[pRow];
      const size_t begin = (pCol == 0) ? 0 : row.ends[pCol - 1];
      pOut.assign(row.chars, begin, row.ends[pCol] - begin);
    }

    /**
     * @brief   Returns the content of all cells of one row.
     */
    std::vector<std::string> GetRow(const size_t pRow) const
    {
      const size_t width = RowWidth(pRow);
      std::vector<std::string> cells;
      cells.reserve(width);
      for (size_t c = 0; c < width; ++c)
      {
        cells.push_back(Get(pRow, c));
      }
      return cells;
    }

    // The following members change the table and throw std::logic_error when it is read-only.

    /**
     * @brief   Throws std::logic_error if the table is read-only. Call before changing a
     *          Document, so that a failed change does not leave partial state behind.
     */
    void RequireEditable() const
    {
      if (mReadOnly)
      {
        throw std::logic_error("document was loaded as read-only (StorageMode::ReadOnly)");
      }
    }

    /**
     * @brief   Replaces the content of one existing cell.
     */
    void Set(const size_t pRow, const size_t pCol, const std::string& pVal)
    {
      RequireEditable();
      CheckCell(pRow, pCol);
      RowBuf& row = mRows[pRow];
      const size_t begin = (pCol == 0) ? 0 : row.ends[pCol - 1];
      const size_t oldSize = row.ends[pCol] - begin;
      CheckOffset(row.chars.size() - oldSize + pVal.size());

      row.chars.replace(begin, oldSize, pVal);
      for (size_t c = pCol; c < row.ends.size(); ++c)
      {
        row.ends[c] = static_cast<uint32_t>(row.ends[c] + pVal.size() - oldSize);
      }
    }

    /**
     * @brief   Truncates or extends a row with empty cells to the given width.
     */
    void ResizeRow(const size_t pRow, const size_t pWidth)
    {
      RequireEditable();
      CheckRow(pRow);
      RowBuf& row = mRows[pRow];
      if (pWidth < row.ends.size())
      {
        const size_t keep = (pWidth == 0) ? 0 : row.ends[pWidth - 1];
        row.chars.resize(keep);
        row.ends.resize(pWidth);
      }
      else
      {
        row.ends.resize(pWidth, static_cast<uint32_t>(row.chars.size()));
      }
    }

    /**
     * @brief   Replaces all cells of one existing row.
     */
    void SetRow(const size_t pRow, const std::vector<std::string>& pCells)
    {
      RequireEditable();
      CheckRow(pRow);
      Assign(mRows[pRow], pCells);
    }

    void InsertRow(const size_t pRow, const std::vector<std::string>& pCells)
    {
      RequireEditable();
      if (pRow > mRows.size())
      {
        throw std::out_of_range("row index " + std::to_string(pRow) + " out of range (rows: " +
                                std::to_string(mRows.size()) + ")");
      }
      RowBuf row;
      Assign(row, pCells);
      mRows.insert(mRows.begin() + static_cast<std::ptrdiff_t>(pRow), std::move(row));
    }

    void AppendRow(const std::vector<std::string>& pCells)
    {
      RequireEditable();
      RowBuf row;
      Assign(row, pCells);
      mRows.push_back(std::move(row));
    }

    void EraseRow(const size_t pRow)
    {
      RequireEditable();
      CheckRow(pRow);
      mRows.erase(mRows.begin() + static_cast<std::ptrdiff_t>(pRow));
    }

    /**
     * @brief   Inserts one cell before the cell at pCol (pCol may equal the row width).
     */
    void InsertCell(const size_t pRow, const size_t pCol, const std::string& pVal)
    {
      RequireEditable();
      CheckRow(pRow);
      RowBuf& row = mRows[pRow];
      if (pCol > row.ends.size())
      {
        throw std::out_of_range("column index " + std::to_string(pCol) + " out of range for row " +
                                std::to_string(pRow) + " (row has " + std::to_string(row.ends.size()) + " cells)");
      }

      const size_t begin = (pCol == 0) ? 0 : row.ends[pCol - 1];
      CheckOffset(row.chars.size() + pVal.size());
      row.chars.insert(begin, pVal);
      for (size_t c = pCol; c < row.ends.size(); ++c)
      {
        row.ends[c] = static_cast<uint32_t>(row.ends[c] + pVal.size());
      }
      row.ends.insert(row.ends.begin() + static_cast<std::ptrdiff_t>(pCol),
                      static_cast<uint32_t>(begin + pVal.size()));
    }

    /**
     * @brief   Removes the cell at pCol.
     */
    void EraseCell(const size_t pRow, const size_t pCol)
    {
      RequireEditable();
      CheckCell(pRow, pCol);
      RowBuf& row = mRows[pRow];
      const size_t begin = (pCol == 0) ? 0 : row.ends[pCol - 1];
      const size_t size = row.ends[pCol] - begin;
      row.chars.erase(begin, size);
      row.ends.erase(row.ends.begin() + static_cast<std::ptrdiff_t>(pCol));
      for (size_t c = pCol; c < row.ends.size(); ++c)
      {
        row.ends[c] = static_cast<uint32_t>(row.ends[c] - size);
      }
    }

    // Loading: rows are built cell by cell by the parser. These are allowed in both representations.

    /**
     * @brief   Starts a new row. Cells are added with AppendCell until EndRow or DiscardRow.
     */
    void BeginRow()
    {
      if (mReadOnly)
      {
        mPendingArenaStart = mArena.size();
        mPendingCellStart = mCellEnds.size();
      }
      else
      {
        mPending.chars.clear();
        mPending.ends.clear();
      }
    }

    void AppendCell(const std::string& pVal)
    {
      if (mReadOnly)
      {
        CheckOffset(mArena.size() + pVal.size());
        mArena += pVal;
        mCellEnds.push_back(static_cast<uint32_t>(mArena.size()));
      }
      else
      {
        CheckOffset(mPending.chars.size() + pVal.size());
        mPending.chars += pVal;
        mPending.ends.push_back(static_cast<uint32_t>(mPending.chars.size()));
      }
    }

    /**
     * @brief   Number of cells appended to the row started by the last BeginRow.
     */
    size_t PendingCellCount() const
    {
      return mReadOnly ? (mCellEnds.size() - mPendingCellStart) : mPending.ends.size();
    }

    void EndRow()
    {
      if (mReadOnly)
      {
        CheckOffset(mCellEnds.size());
        mRowEnds.push_back(static_cast<uint32_t>(mCellEnds.size()));
      }
      else
      {
        const size_t width = mPending.ends.size();
        mPending.chars.shrink_to_fit();
        mRows.push_back(std::move(mPending));
        mPending = RowBuf();
        mPending.ends.reserve(width);
      }
    }

    /**
     * @brief   Drops the cells of the row started by the last BeginRow.
     */
    void DiscardRow()
    {
      if (mReadOnly)
      {
        mArena.resize(mPendingArenaStart);
        mCellEnds.resize(mPendingCellStart);
      }
      else
      {
        mPending.chars.clear();
        mPending.ends.clear();
      }
    }

    /**
     * @brief   Releases spare capacity once loading is complete.
     */
    void ShrinkToFit()
    {
      mArena.shrink_to_fit();
      mCellEnds.shrink_to_fit();
      mRowEnds.shrink_to_fit();
    }

  private:
    struct RowBuf
    {
      RowBuf() : chars(), ends() {}
      std::string chars;             // cell contents, back to back
      std::vector<uint32_t> ends;    // end offset in chars of each cell
    };

    void CheckRow(const size_t pRow) const
    {
      if (pRow >= RowCount())
      {
        throw std::out_of_range("row index " + std::to_string(pRow) + " out of range (rows: " +
                                std::to_string(RowCount()) + ")");
      }
    }

    void CheckCell(const size_t pRow, const size_t pCol) const
    {
      const size_t width = RowWidth(pRow);
      if (pCol >= width)
      {
        throw std::out_of_range("column index " + std::to_string(pCol) + " out of range for row " +
                                std::to_string(pRow) + " (row has " + std::to_string(width) + " cells)");
      }
    }

    size_t RowFirstCell(const size_t pRow) const
    {
      return (pRow == 0) ? 0 : mRowEnds[pRow - 1];
    }

    static void CheckOffset(const size_t pSize)
    {
      if (pSize > static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
      {
        throw std::length_error("cell data exceeds the 4 GiB limit of the table storage");
      }
    }

    static void Assign(RowBuf& pRow, const std::vector<std::string>& pCells)
    {
      size_t total = 0;
      for (const std::string& cell : pCells)
      {
        total += cell.size();
      }
      CheckOffset(total);

      pRow.chars.clear();
      pRow.ends.clear();
      pRow.chars.reserve(total);
      pRow.ends.reserve(pCells.size());
      for (const std::string& cell : pCells)
      {
        pRow.chars += cell;
        pRow.ends.push_back(static_cast<uint32_t>(pRow.chars.size()));
      }
    }

    bool mReadOnly;
    std::vector<RowBuf> mRows;          // editable representation
    std::string mArena;                 // read-only representation: all cell text
    std::vector<uint32_t> mCellEnds;    // read-only: end offset in mArena of each cell
    std::vector<uint32_t> mRowEnds;     // read-only: cumulative cell count after each row
    RowBuf mPending;                    // editable: row being loaded
    size_t mPendingArenaStart;          // read-only: arena size when the row being loaded began
    size_t mPendingCellStart;           // read-only: cell count when the row being loaded began
  };

  /**
   * @brief     Class representing a CSV document.
   */
  class Document
  {
  public:
    /**
     * @brief   Constructor
     * @param   pPath                 specifies the path of an existing CSV-file to populate the Document
     *                                data with.
     * @param   pLabelParams          specifies which row and column should be treated as labels.
     * @param   pSeparatorParams      specifies which field and row separators should be used.
     * @param   pConverterParams      specifies how invalid numbers (including empty strings) should be
     *                                handled.
     * @param   pLineReaderParams     specifies how special line formats should be treated.
     */
    explicit Document(const std::string& pPath = std::string(),
                      const LabelParams& pLabelParams = LabelParams(),
                      const SeparatorParams& pSeparatorParams = SeparatorParams(),
                      const ConverterParams& pConverterParams = ConverterParams(),
                      const LineReaderParams& pLineReaderParams = LineReaderParams(),
                      const StorageMode pStorageMode = StorageMode::Editable)
      : mPath(pPath)
      , mLabelParams(pLabelParams)
      , mSeparatorParams(pSeparatorParams)
      , mConverterParams(pConverterParams)
      , mLineReaderParams(pLineReaderParams)
      , mData()
      , mColumnNames()
      , mRowNames()
    {
      if (!mPath.empty())
      {
        ReadCsv(pStorageMode == StorageMode::ReadOnly);
      }
    }

    /**
     * @brief   Constructor
     * @param   pStream               specifies a binary input stream to read CSV data from.
     * @param   pLabelParams          specifies which row and column should be treated as labels.
     * @param   pSeparatorParams      specifies which field and row separators should be used.
     * @param   pConverterParams      specifies how invalid numbers (including empty strings) should be
     *                                handled.
     * @param   pLineReaderParams     specifies how special line formats should be treated.
     */
    explicit Document(std::istream& pStream,
                      const LabelParams& pLabelParams = LabelParams(),
                      const SeparatorParams& pSeparatorParams = SeparatorParams(),
                      const ConverterParams& pConverterParams = ConverterParams(),
                      const LineReaderParams& pLineReaderParams = LineReaderParams(),
                      const StorageMode pStorageMode = StorageMode::Editable)
      : mPath()
      , mLabelParams(pLabelParams)
      , mSeparatorParams(pSeparatorParams)
      , mConverterParams(pConverterParams)
      , mLineReaderParams(pLineReaderParams)
      , mData()
      , mColumnNames()
      , mRowNames()
    {
      ReadCsv(pStream, pStorageMode == StorageMode::ReadOnly);
    }

    /**
     * @brief   Read Document data from file.
     * @param   pPath                 specifies the path of an existing CSV-file to populate the Document
     *                                data with.
     * @param   pLabelParams          specifies which row and column should be treated as labels.
     * @param   pSeparatorParams      specifies which field and row separators should be used.
     * @param   pConverterParams      specifies how invalid numbers (including empty strings) should be
     *                                handled.
     * @param   pLineReaderParams     specifies how special line formats should be treated.
     */
    void Load(const std::string& pPath,
              const LabelParams& pLabelParams = LabelParams(),
              const SeparatorParams& pSeparatorParams = SeparatorParams(),
              const ConverterParams& pConverterParams = ConverterParams(),
              const LineReaderParams& pLineReaderParams = LineReaderParams(),
              const StorageMode pStorageMode = StorageMode::Editable)
    {
      mPath = pPath;
      mLabelParams = pLabelParams;
      mSeparatorParams = pSeparatorParams;
      mConverterParams = pConverterParams;
      mLineReaderParams = pLineReaderParams;
      ReadCsv(pStorageMode == StorageMode::ReadOnly);
    }

    /**
     * @brief   Read Document data from stream.
     * @param   pStream               specifies a binary input stream to read CSV data from.
     * @param   pLabelParams          specifies which row and column should be treated as labels.
     * @param   pSeparatorParams      specifies which field and row separators should be used.
     * @param   pConverterParams      specifies how invalid numbers (including empty strings) should be
     *                                handled.
     * @param   pLineReaderParams     specifies how special line formats should be treated.
     */
    void Load(std::istream& pStream,
              const LabelParams& pLabelParams = LabelParams(),
              const SeparatorParams& pSeparatorParams = SeparatorParams(),
              const ConverterParams& pConverterParams = ConverterParams(),
              const LineReaderParams& pLineReaderParams = LineReaderParams(),
              const StorageMode pStorageMode = StorageMode::Editable)
    {
      mPath = "";
      mLabelParams = pLabelParams;
      mSeparatorParams = pSeparatorParams;
      mConverterParams = pConverterParams;
      mLineReaderParams = pLineReaderParams;
      ReadCsv(pStream, pStorageMode == StorageMode::ReadOnly);
    }

    /**
     * @brief   Write Document data to file.
     * @param   pPath                 optionally specifies the path where the CSV-file will be created
     *                                (if not specified, the original path provided when creating or
     *                                loading the Document data will be used).
     */
    void Save(const std::string& pPath = std::string())
    {
      if (!pPath.empty())
      {
        mPath = pPath;
      }
      WriteCsv();
    }

    /**
     * @brief   Write Document data to stream.
     * @param   pStream               specifies a binary output stream to write the data to.
     */
    void Save(std::ostream& pStream) const
    {
      WriteCsv(pStream);
    }

    /**
     * @brief   Clears loaded Document data.
     *
     */
    void Clear()
    {
      mData.Clear();
      mColumnNames.clear();
      mRowNames.clear();
      mIsUtf16 = false;
      mIsLE = false;
      mHasUtf8BOM = false;
    }

    /**
     * @brief   Get column index by name. The result is undefined if the document contains
     *          multiple columns sharing the same label name. Column indices exceeding
     *          INT_MAX are not supported.
     * @param   pColumnName           column label name.
     * @returns zero-based column index.
     */
    int GetColumnIdx(const std::string& pColumnName) const
    {
      if (mLabelParams.mColumnNameIdx >= 0)
      {
        if (mColumnNames.find(pColumnName) != mColumnNames.end())
        {
          return static_cast<int>(mColumnNames.at(pColumnName)) - (mLabelParams.mRowNameIdx + 1);
        }
      }
      return -1;
    }

    /**
     * @brief   Get column by index.
     * @param   pColumnIdx            zero-based column index.
     * @returns vector of column data.
     */
    template<typename T>
    std::vector<T> GetColumn(const size_t pColumnIdx) const
    {
      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);
      std::vector<T> column;
      Converter<T> converter(mConverterParams);
      for (size_t dataRowIdx = 0; dataRowIdx < mData.RowCount(); ++dataRowIdx)
      {
        if (static_cast<std::ptrdiff_t>(dataRowIdx) > mLabelParams.mColumnNameIdx)
        {
          const size_t width = mData.RowWidth(dataRowIdx);
          if (dataColumnIdx < width)
          {
            T val;
            converter.ToVal(mData.Get(dataRowIdx, dataColumnIdx), val);
            column.push_back(val);
          }
          else
          {
            const std::string errStr = "requested column index " +
              std::to_string(pColumnIdx) + " >= " +
              std::to_string(width - GetDataColumnIndex(0)) +
              " (number of columns on row index " +
              std::to_string(static_cast<std::ptrdiff_t>(dataRowIdx) -
                             (mLabelParams.mColumnNameIdx + 1)) + ")";
            throw std::out_of_range(errStr);
          }
        }
      }
      return column;
    }

    /**
     * @brief   Get column by index.
     * @param   pColumnIdx            zero-based column index.
     * @param   pToVal                conversion function.
     * @returns vector of column data.
     */
    template<typename T>
    std::vector<T> GetColumn(const size_t pColumnIdx, ConvFunc<T> pToVal) const
    {
      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);
      std::vector<T> column;
      for (size_t dataRowIdx = 0; dataRowIdx < mData.RowCount(); ++dataRowIdx)
      {
        if (static_cast<std::ptrdiff_t>(dataRowIdx) > mLabelParams.mColumnNameIdx)
        {
          T val;
          pToVal(mData.Get(dataRowIdx, dataColumnIdx), val);
          column.push_back(val);
        }
      }
      return column;
    }

    /**
     * @brief   Get column by name.
     * @param   pColumnName           column label name.
     * @returns vector of column data.
     */
    template<typename T>
    std::vector<T> GetColumn(const std::string& pColumnName) const
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }
      return GetColumn<T>(static_cast<size_t>(columnIdx));
    }

    /**
     * @brief   Get column by name.
     * @param   pColumnName           column label name.
     * @param   pToVal                conversion function.
     * @returns vector of column data.
     */
    template<typename T>
    std::vector<T> GetColumn(const std::string& pColumnName, ConvFunc<T> pToVal) const
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }
      return GetColumn<T>(static_cast<size_t>(columnIdx), pToVal);
    }

    /**
     * @brief   Set column by index.
     * @param   pColumnIdx            zero-based column index.
     * @param   pColumn               vector of column data.
     */
    template<typename T>
    void SetColumn(const size_t pColumnIdx, const std::vector<T>& pColumn)
    {
      mData.RequireEditable();
      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);

      while (GetDataRowIndex(pColumn.size()) > GetDataRowCount())
      {
        mData.AppendRow(std::vector<std::string>(GetDataColumnCount()));
      }

      if ((dataColumnIdx + 1) > GetDataColumnCount())
      {
        for (size_t dataRowIdx = 0; dataRowIdx < mData.RowCount(); ++dataRowIdx)
        {
          if (static_cast<std::ptrdiff_t>(dataRowIdx) >= mLabelParams.mColumnNameIdx)
          {
            if (mData.RowWidth(dataRowIdx) < (dataColumnIdx + 1))
            {
              mData.ResizeRow(dataRowIdx, dataColumnIdx + 1);
            }
          }
        }
      }

      Converter<T> converter(mConverterParams);
      for (size_t i = 0; i < pColumn.size(); ++i)
      {
        std::string str;
        converter.ToStr(pColumn[i], str);
        mData.Set(i + static_cast<size_t>(mLabelParams.mColumnNameIdx + 1), dataColumnIdx, str);
      }
    }

    /**
     * @brief   Set column by name.
     * @param   pColumnName           column label name.
     * @param   pColumn               vector of column data.
     */
    template<typename T>
    void SetColumn(const std::string& pColumnName, const std::vector<T>& pColumn)
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }
      SetColumn<T>(static_cast<size_t>(columnIdx), pColumn);
    }

    /**
     * @brief   Remove column by index.
     * @param   pColumnIdx            zero-based column index.
     */
    void RemoveColumn(const size_t pColumnIdx)
    {
      mData.RequireEditable();
      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);
      for (size_t dataRowIdx = 0; dataRowIdx < mData.RowCount(); ++dataRowIdx)
      {
        if (static_cast<std::ptrdiff_t>(dataRowIdx) >= mLabelParams.mColumnNameIdx)
        {
          if (dataColumnIdx < mData.RowWidth(dataRowIdx))
          {
            mData.EraseCell(dataRowIdx, dataColumnIdx);
          }
          else
          {
            const std::string errStr = "column out of range: " +
              std::to_string(pColumnIdx) + " (on row " +
              std::to_string(dataRowIdx) +
              ")";
            throw std::out_of_range(errStr);
          }
        }
      }

      UpdateColumnNames();
    }

    /**
     * @brief   Remove column by name.
     * @param   pColumnName           column label name.
     */
    void RemoveColumn(const std::string& pColumnName)
    {
      int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }

      RemoveColumn(static_cast<size_t>(columnIdx));
    }

    /**
     * @brief   Insert column at specified index.
     * @param   pColumnIdx            zero-based column index.
     * @param   pColumn               vector of column data (optional argument).
     * @param   pColumnName           column label name (optional argument).
     */
    template<typename T>
    void InsertColumn(const size_t pColumnIdx, const std::vector<T>& pColumn = std::vector<T>(),
                      const std::string& pColumnName = std::string())
    {
      mData.RequireEditable();
      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);

      std::vector<std::string> column;
      if (pColumn.empty())
      {
        column.resize(GetDataRowCount());
      }
      else
      {
        column.resize(GetDataRowIndex(pColumn.size()));
        Converter<T> converter(mConverterParams);
        for (size_t i = 0; i < pColumn.size(); ++i)
        {
          std::string str;
          converter.ToStr(pColumn[i], str);
          const size_t rowIdx = i + static_cast<size_t>(mLabelParams.mColumnNameIdx + 1);
          column.at(rowIdx) = str;
        }
      }

      while (column.size() > GetDataRowCount())
      {
        const size_t columnCount = std::max<size_t>(static_cast<size_t>(mLabelParams.mColumnNameIdx + 1),
                                                    GetDataColumnCount());
        mData.AppendRow(std::vector<std::string>(columnCount));
      }

      for (size_t dataRowIdx = 0; dataRowIdx < mData.RowCount(); ++dataRowIdx)
      {
        if (static_cast<std::ptrdiff_t>(dataRowIdx) >= mLabelParams.mColumnNameIdx)
        {
          if (dataColumnIdx <= mData.RowWidth(dataRowIdx))
          {
            mData.InsertCell(dataRowIdx, dataColumnIdx, column.at(dataRowIdx));
          }
          else
          {
            const std::string errStr = "column out of range: " +
              std::to_string(pColumnIdx) + " (on row " +
              std::to_string(dataRowIdx) +
              ")";
            throw std::out_of_range(errStr);
          }
        }
      }

      if (!pColumnName.empty())
      {
        SetColumnName(pColumnIdx, pColumnName);
      }

      UpdateColumnNames();
    }

    /**
     * @brief   Get number of data columns (excluding label columns).
     * @returns column count.
     */
    size_t GetColumnCount() const
    {
      const size_t firstRow = static_cast<size_t>((mLabelParams.mColumnNameIdx >= 0) ? mLabelParams.mColumnNameIdx : 0);
      const std::ptrdiff_t count =
        static_cast<std::ptrdiff_t>((mData.RowCount() > firstRow) ? mData.RowWidth(firstRow) : 0) -
        (mLabelParams.mRowNameIdx + 1);
      return (count >= 0) ? static_cast<size_t>(count) : 0;
    }

    /**
     * @brief   Get row index by name. The result is undefined if the document contains
     *          multiple rows sharing the same label name. Row indices exceeding INT_MAX
     *          are not supported.
     * @param   pRowName              row label name.
     * @returns zero-based row index.
     */
    int GetRowIdx(const std::string& pRowName) const
    {
      if (mLabelParams.mRowNameIdx >= 0)
      {
        if (mRowNames.find(pRowName) != mRowNames.end())
        {
          return static_cast<int>(mRowNames.at(pRowName)) - (mLabelParams.mColumnNameIdx + 1);
        }
      }
      return -1;
    }

    /**
     * @brief   Get row by index.
     * @param   pRowIdx               zero-based row index.
     * @returns vector of row data.
     */
    template<typename T>
    std::vector<T> GetRow(const size_t pRowIdx) const
    {
      const size_t dataRowIdx = GetDataRowIndex(pRowIdx);
      std::vector<T> row;
      Converter<T> converter(mConverterParams);
      const size_t width = mData.RowWidth(dataRowIdx);
      for (size_t dataColumnIdx = 0; dataColumnIdx < width; ++dataColumnIdx)
      {
        if (static_cast<std::ptrdiff_t>(dataColumnIdx) > mLabelParams.mRowNameIdx)
        {
          T val;
          converter.ToVal(mData.Get(dataRowIdx, dataColumnIdx), val);
          row.push_back(val);
        }
      }
      return row;
    }

    /**
     * @brief   Get row by index.
     * @param   pRowIdx               zero-based row index.
     * @param   pToVal                conversion function.
     * @returns vector of row data.
     */
    template<typename T>
    std::vector<T> GetRow(const size_t pRowIdx, ConvFunc<T> pToVal) const
    {
      const size_t dataRowIdx = GetDataRowIndex(pRowIdx);
      std::vector<T> row;
      const size_t width = mData.RowWidth(dataRowIdx);
      for (size_t dataColumnIdx = 0; dataColumnIdx < width; ++dataColumnIdx)
      {
        if (static_cast<std::ptrdiff_t>(dataColumnIdx) > mLabelParams.mRowNameIdx)
        {
          T val;
          pToVal(mData.Get(dataRowIdx, dataColumnIdx), val);
          row.push_back(val);
        }
      }
      return row;
    }

    /**
     * @brief   Get row by name.
     * @param   pRowName              row label name.
     * @returns vector of row data.
     */
    template<typename T>
    std::vector<T> GetRow(const std::string& pRowName) const
    {
      int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }
      return GetRow<T>(static_cast<size_t>(rowIdx));
    }

    /**
     * @brief   Get row by name.
     * @param   pRowName              row label name.
     * @param   pToVal                conversion function.
     * @returns vector of row data.
     */
    template<typename T>
    std::vector<T> GetRow(const std::string& pRowName, ConvFunc<T> pToVal) const
    {
      int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }
      return GetRow<T>(static_cast<size_t>(rowIdx), pToVal);
    }

    /**
     * @brief   Set row by index.
     * @param   pRowIdx               zero-based row index.
     * @param   pRow                  vector of row data.
     */
    template<typename T>
    void SetRow(const size_t pRowIdx, const std::vector<T>& pRow)
    {
      mData.RequireEditable();
      const size_t dataRowIdx = GetDataRowIndex(pRowIdx);

      while ((dataRowIdx + 1) > GetDataRowCount())
      {
        mData.AppendRow(std::vector<std::string>(GetDataColumnCount()));
      }

      if (GetDataColumnIndex(pRow.size()) > GetDataColumnCount())
      {
        for (size_t r = 0; r < mData.RowCount(); ++r)
        {
          if (static_cast<std::ptrdiff_t>(r) >= mLabelParams.mColumnNameIdx)
          {
            if (mData.RowWidth(r) < GetDataColumnIndex(pRow.size()))
            {
              mData.ResizeRow(r, GetDataColumnIndex(pRow.size()));
            }
          }
        }
      }

      Converter<T> converter(mConverterParams);
      for (size_t i = 0; i < pRow.size(); ++i)
      {
        std::string str;
        converter.ToStr(pRow[i], str);
        mData.Set(dataRowIdx, i + static_cast<size_t>(mLabelParams.mRowNameIdx + 1), str);
      }
    }

    /**
     * @brief   Set row by name.
     * @param   pRowName              row label name.
     * @param   pRow                  vector of row data.
     */
    template<typename T>
    void SetRow(const std::string& pRowName, const std::vector<T>& pRow)
    {
      int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }
      return SetRow<T>(static_cast<size_t>(rowIdx), pRow);
    }

    /**
     * @brief   Remove row by index.
     * @param   pRowIdx               zero-based row index.
     */
    void RemoveRow(const size_t pRowIdx)
    {
      mData.RequireEditable();
      const size_t dataRowIdx = GetDataRowIndex(pRowIdx);
      if (dataRowIdx < mData.RowCount())
      {
        mData.EraseRow(dataRowIdx);
      }
      else
      {
        const std::string errStr = "row out of range: " +
          std::to_string(pRowIdx);
        throw std::out_of_range(errStr);
      }

      UpdateRowNames();
    }

    /**
     * @brief   Remove row by name.
     * @param   pRowName              row label name.
     */
    void RemoveRow(const std::string& pRowName)
    {
      int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }

      RemoveRow(static_cast<size_t>(rowIdx));
    }

    /**
     * @brief   Insert row at specified index.
     * @param   pRowIdx               zero-based row index.
     * @param   pRow                  vector of row data (optional argument).
     * @param   pRowName              row label name (optional argument).
     */
    template<typename T>
    void InsertRow(const size_t pRowIdx, const std::vector<T>& pRow = std::vector<T>(),
                   const std::string& pRowName = std::string())
    {
      mData.RequireEditable();
      const size_t rowIdx = GetDataRowIndex(pRowIdx);

      std::vector<std::string> row;
      if (pRow.empty())
      {
        row.resize(GetDataColumnCount());
      }
      else
      {
        row.resize(GetDataColumnIndex(pRow.size()));
        Converter<T> converter(mConverterParams);
        for (size_t i = 0; i < pRow.size(); ++i)
        {
          std::string str;
          converter.ToStr(pRow[i], str);
          row.at(i + static_cast<size_t>(mLabelParams.mRowNameIdx + 1)) = str;
        }
      }

      while (rowIdx > GetDataRowCount())
      {
        mData.AppendRow(std::vector<std::string>(GetDataColumnCount()));
      }

      mData.InsertRow(rowIdx, row);

      if (!pRowName.empty())
      {
        SetRowName(pRowIdx, pRowName);
      }

      UpdateRowNames();
    }

    /**
     * @brief   Get number of data rows (excluding label rows).
     * @returns row count.
     */
    size_t GetRowCount() const
    {
      const std::ptrdiff_t count = static_cast<std::ptrdiff_t>(mData.RowCount()) -
        (mLabelParams.mColumnNameIdx + 1);
      return (count >= 0) ? static_cast<size_t>(count) : 0;
    }

    /**
     * @brief   Get cell by index.
     * @param   pColumnIdx            zero-based column index.
     * @param   pRowIdx               zero-based row index.
     * @returns cell data.
     */
    template<typename T>
    T GetCell(const size_t pColumnIdx, const size_t pRowIdx) const
    {
      T val;
      GetCellTo(pColumnIdx, pRowIdx, val, std::is_same<T, std::string>());
      return val;
    }

    /**
     * @brief   Get cell by index.
     * @param   pColumnIdx            zero-based column index.
     * @param   pRowIdx               zero-based row index.
     * @param   pToVal                conversion function.
     * @returns cell data.
     */
    template<typename T>
    T GetCell(const size_t pColumnIdx, const size_t pRowIdx, ConvFunc<T> pToVal) const
    {
      T val;
      pToVal(GetCellValue(pColumnIdx, pRowIdx), val);
      return val;
    }

    /**
     * @brief   Get cell by name.
     * @param   pColumnName           column label name.
     * @param   pRowName              row label name.
     * @returns cell data.
     */
    template<typename T>
    T GetCell(const std::string& pColumnName, const std::string& pRowName) const
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }

      const int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }

      return GetCell<T>(static_cast<size_t>(columnIdx), static_cast<size_t>(rowIdx));
    }

    /**
     * @brief   Get cell by name.
     * @param   pColumnName           column label name.
     * @param   pRowName              row label name.
     * @param   pToVal                conversion function.
     * @returns cell data.
     */
    template<typename T>
    T GetCell(const std::string& pColumnName, const std::string& pRowName, ConvFunc<T> pToVal) const
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }

      const int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }

      return GetCell<T>(static_cast<size_t>(columnIdx), static_cast<size_t>(rowIdx), pToVal);
    }

    /**
     * @brief   Get cell by column name and row index.
     * @param   pColumnName           column label name.
     * @param   pRowIdx               zero-based row index.
     * @returns cell data.
     */
    template<typename T>
    T GetCell(const std::string& pColumnName, const size_t pRowIdx) const
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }

      return GetCell<T>(static_cast<size_t>(columnIdx), pRowIdx);
    }

    /**
     * @brief   Get cell by column name and row index.
     * @param   pColumnName           column label name.
     * @param   pRowIdx               zero-based row index.
     * @param   pToVal                conversion function.
     * @returns cell data.
     */
    template<typename T>
    T GetCell(const std::string& pColumnName, const size_t pRowIdx, ConvFunc<T> pToVal) const
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }

      return GetCell<T>(static_cast<size_t>(columnIdx), pRowIdx, pToVal);
    }

    /**
     * @brief   Get cell by column index and row name.
     * @param   pColumnIdx            zero-based column index.
     * @param   pRowName              row label name.
     * @returns cell data.
     */
    template<typename T>
    T GetCell(const size_t pColumnIdx, const std::string& pRowName) const
    {
      const int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }

      return GetCell<T>(pColumnIdx, static_cast<size_t>(rowIdx));
    }

    /**
     * @brief   Get cell by column index and row name.
     * @param   pColumnIdx            zero-based column index.
     * @param   pRowName              row label name.
     * @param   pToVal                conversion function.
     * @returns cell data.
     */
    template<typename T>
    T GetCell(const size_t pColumnIdx, const std::string& pRowName, ConvFunc<T> pToVal) const
    {
      const int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }

      return GetCell<T>(pColumnIdx, static_cast<size_t>(rowIdx), pToVal);
    }

    /**
     * @brief   Set cell by index.
     * @param   pRowIdx               zero-based row index.
     * @param   pColumnIdx            zero-based column index.
     * @param   pCell                 cell data.
     */
    template<typename T>
    void SetCell(const size_t pColumnIdx, const size_t pRowIdx, const T& pCell)
    {
      mData.RequireEditable();
      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);
      const size_t dataRowIdx = GetDataRowIndex(pRowIdx);

      while ((dataRowIdx + 1) > GetDataRowCount())
      {
        mData.AppendRow(std::vector<std::string>(GetDataColumnCount()));
      }

      if ((dataColumnIdx + 1) > GetDataColumnCount())
      {
        for (size_t r = 0; r < mData.RowCount(); ++r)
        {
          if (static_cast<std::ptrdiff_t>(r) >= mLabelParams.mColumnNameIdx)
          {
            if (mData.RowWidth(r) < (dataColumnIdx + 1))
            {
              mData.ResizeRow(r, dataColumnIdx + 1);
            }
          }
        }
      }

      std::string str;
      Converter<T> converter(mConverterParams);
      converter.ToStr(pCell, str);
      mData.Set(dataRowIdx, dataColumnIdx, str);
    }

    /**
     * @brief   Set cell by name.
     * @param   pColumnName           column label name.
     * @param   pRowName              row label name.
     * @param   pCell                 cell data.
     */
    template<typename T>
    void SetCell(const std::string& pColumnName, const std::string& pRowName, const T& pCell)
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }

      const int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }

      SetCell<T>(static_cast<size_t>(columnIdx), static_cast<size_t>(rowIdx), pCell);
    }

    /**
     * @brief   Set cell by column index and row name.
     * @param   pColumnIdx            zero-based column index.
     * @param   pRowName              row label name.
     * @param   pCell                 cell data.
     */
    template<typename T>
    void SetCell(const size_t pColumnIdx, const std::string& pRowName, const T& pCell)
    {
      const int rowIdx = GetRowIdx(pRowName);
      if (rowIdx < 0)
      {
        throw std::out_of_range("row not found: " + pRowName);
      }

      SetCell<T>(pColumnIdx, static_cast<size_t>(rowIdx), pCell);
    }

    /**
     * @brief   Set cell by column name and row index.
     * @param   pColumnName           column label name.
     * @param   pRowIdx               zero-based row index.
     * @param   pCell                 cell data.
     */
    template<typename T>
    void SetCell(const std::string& pColumnName, const size_t pRowIdx, const T& pCell)
    {
      const int columnIdx = GetColumnIdx(pColumnName);
      if (columnIdx < 0)
      {
        throw std::out_of_range("column not found: " + pColumnName);
      }

      SetCell<T>(static_cast<size_t>(columnIdx), pRowIdx, pCell);
    }

    /**
     * @brief   Get column name
     * @param   pColumnIdx            zero-based column index.
     * @returns column name.
     */
    std::string GetColumnName(const size_t pColumnIdx) const
    {
      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);
      if (mLabelParams.mColumnNameIdx < 0)
      {
        throw std::out_of_range("column name row index < 0: " + std::to_string(mLabelParams.mColumnNameIdx));
      }

      return mData.Get(static_cast<size_t>(mLabelParams.mColumnNameIdx), dataColumnIdx);
    }

    /**
     * @brief   Set column name
     * @param   pColumnIdx            zero-based column index.
     * @param   pColumnName           column name.
     */
    void SetColumnName(size_t pColumnIdx, const std::string& pColumnName)
    {
      mData.RequireEditable();
      if (mLabelParams.mColumnNameIdx < 0)
      {
        throw std::out_of_range("column name row index < 0: " + std::to_string(mLabelParams.mColumnNameIdx));
      }

      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);

      // remove old name from map before adding new one
      const size_t nameRowIdx = static_cast<size_t>(mLabelParams.mColumnNameIdx);
      if ((nameRowIdx < mData.RowCount()) && (dataColumnIdx < mData.RowWidth(nameRowIdx)))
      {
        const std::string oldName = mData.Get(nameRowIdx, dataColumnIdx);
        mColumnNames.erase(oldName);
      }
      mColumnNames[pColumnName] = dataColumnIdx;

      // increase table size if necessary:
      while (mData.RowCount() < (nameRowIdx + 1))
      {
        mData.AppendRow(std::vector<std::string>());
      }

      if (dataColumnIdx >= mData.RowWidth(nameRowIdx))
      {
        mData.ResizeRow(nameRowIdx, dataColumnIdx + 1);
      }

      mData.Set(nameRowIdx, dataColumnIdx, pColumnName);
    }

    /**
     * @brief   Get column names
     * @returns vector of column names.
     */
    std::vector<std::string> GetColumnNames() const
    {
      if (mLabelParams.mColumnNameIdx >= 0)
      {
        const size_t nameRowIdx = static_cast<size_t>(mLabelParams.mColumnNameIdx);
        const size_t offset = static_cast<size_t>(mLabelParams.mRowNameIdx + 1);
        const size_t width = mData.RowWidth(nameRowIdx);
        if (offset <= width)
        {
          std::vector<std::string> names;
          for (size_t c = offset; c < width; ++c)
          {
            names.push_back(mData.Get(nameRowIdx, c));
          }
          return names;
        }
      }

      return std::vector<std::string>();
    }

    /**
     * @brief   Get row name
     * @param   pRowIdx               zero-based column index.
     * @returns row name.
     */
    std::string GetRowName(const size_t pRowIdx) const
    {
      const size_t dataRowIdx = GetDataRowIndex(pRowIdx);
      if (mLabelParams.mRowNameIdx < 0)
      {
        throw std::out_of_range("row name column index < 0: " + std::to_string(mLabelParams.mRowNameIdx));
      }

      return mData.Get(dataRowIdx, static_cast<size_t>(mLabelParams.mRowNameIdx));
    }

    /**
     * @brief   Set row name
     * @param   pRowIdx               zero-based row index.
     * @param   pRowName              row name.
     */
    void SetRowName(size_t pRowIdx, const std::string& pRowName)
    {
      mData.RequireEditable();
      const size_t dataRowIdx = GetDataRowIndex(pRowIdx);

      // remove old name from map before adding new one
      if ((mLabelParams.mRowNameIdx >= 0) && (dataRowIdx < mData.RowCount()) &&
          (static_cast<size_t>(mLabelParams.mRowNameIdx) < mData.RowWidth(dataRowIdx)))
      {
        const std::string oldName = mData.Get(dataRowIdx, static_cast<size_t>(mLabelParams.mRowNameIdx));
        mRowNames.erase(oldName);
      }
      mRowNames[pRowName] = dataRowIdx;
      if (mLabelParams.mRowNameIdx < 0)
      {
        throw std::out_of_range("row name column index < 0: " + std::to_string(mLabelParams.mRowNameIdx));
      }

      // increase table size if necessary:
      while (dataRowIdx >= mData.RowCount())
      {
        mData.AppendRow(std::vector<std::string>());
      }

      const size_t nameColIdx = static_cast<size_t>(mLabelParams.mRowNameIdx);
      if (nameColIdx >= mData.RowWidth(dataRowIdx))
      {
        mData.ResizeRow(dataRowIdx, nameColIdx + 1);
      }

      mData.Set(dataRowIdx, nameColIdx, pRowName);
    }

    /**
     * @brief   Get row names
     * @returns vector of row names.
     */
    std::vector<std::string> GetRowNames() const
    {
      std::vector<std::string> rownames;
      if (mLabelParams.mRowNameIdx >= 0)
      {
        for (size_t r = 0; r < mData.RowCount(); ++r)
        {
          if (static_cast<std::ptrdiff_t>(r) > mLabelParams.mColumnNameIdx)
          {
            rownames.push_back(mData.Get(r, static_cast<size_t>(mLabelParams.mRowNameIdx)));
          }
        }
      }
      return rownames;
    }

  private:
    void ReadCsv(const bool pReadOnly)
    {
      std::ifstream stream;
      stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      stream.open(mPath, std::ios::binary);
      ReadCsv(stream, pReadOnly);
    }

    void ReadCsv(std::istream& pStream, const bool pReadOnly)
    {
      Clear();
      mData.Reset(pReadOnly);
      pStream.seekg(0, std::ios::end);
      std::streamsize length = pStream.tellg();
      pStream.seekg(0, std::ios::beg);

      std::vector<char> bom2b(2, '\0');
      if (length >= 2)
      {
        pStream.read(bom2b.data(), 2);
        pStream.seekg(0, std::ios::beg);
      }

      static const std::vector<char> bomU16le = { '\xff', '\xfe' };
      static const std::vector<char> bomU16be = { '\xfe', '\xff' };
      if ((bom2b == bomU16le) || (bom2b == bomU16be))
      {
        mIsUtf16 = true;
        mIsLE = (bom2b == bomU16le);

        std::string utf8;
        {
          std::vector<char> buffer(static_cast<size_t>(length));
          pStream.read(buffer.data(), length);

          // skip byte order mark
          utf8 = Utf16ToUtf8(buffer.data() + 2, static_cast<size_t>(length) - 2, mIsLE);
        }  // raw UTF-16 buffer released here, before parsing

        // parse the UTF-8 text in place, without a second copy in a stringstream
        StringViewBuf view(utf8);
        std::istream utf8Stream(&view);
        ParseCsv(utf8Stream, static_cast<std::streamsize>(utf8.size()));
      }
      else
      {
        // check for UTF-8 Byte order mark and skip it when found
        if (length >= 3)
        {
          std::vector<char> bom3b(3, '\0');
          pStream.read(bom3b.data(), 3);

          if (bom3b != s_Utf8BOM)
          {
            // file does not start with a UTF-8 Byte order mark
            pStream.seekg(0, std::ios::beg);
          }
          else
          {
            // file did start with a UTF-8 Byte order mark, simply skip it
            length -= 3;
            mHasUtf8BOM = true;
          }
        }

        ParseCsv(pStream, length);
      }
    }

    static inline void CheckMaxCount(const size_t pCount)
    {
#ifdef RAPIDCSV_MAX_COUNT
      static_assert(RAPIDCSV_MAX_COUNT > 0, "RAPIDCSV_MAX_COUNT must be a positive value");
      static const size_t s_MaxCount = static_cast<size_t>(RAPIDCSV_MAX_COUNT);
      if (pCount > s_MaxCount)
      {
        const std::string errStr = "max count exceeded: " + std::to_string(s_MaxCount);
        throw std::out_of_range(errStr);
      }
#else
      (void)pCount;
#endif
    }

    void ParseCsv(std::istream& pStream, std::streamsize p_FileLength)
    {
      const std::streamsize bufLength = 64 * 1024;
      std::vector<char> buffer(bufLength);
      std::string cell;
      size_t cellsInRow = 0;   // cells seen in the current record
      bool skipRow = false;    // the current record is a comment line to be dropped
      bool quoted = false;
      int cr = 0;
      int lf = 0;

      // Completes the current cell and adds it to the current record.
      const auto addCell = [&]() {
        std::string value = FinishCell(std::move(cell));
        cell.clear();
        if (cellsInRow == 0)
        {
          mData.BeginRow();
          skipRow = mLineReaderParams.mSkipCommentLines && !value.empty() &&
            (value[0] == mLineReaderParams.mCommentPrefix);
        }
        CheckMaxCount(cellsInRow + 1);
        if (!skipRow)
        {
          mData.AppendCell(value);
        }
        ++cellsInRow;
      };

      // Completes the current record: it is stored, or dropped if it is a comment line.
      const auto endRecord = [&]() {
        if (skipRow)
        {
          mData.DiscardRow();
        }
        else
        {
          CheckMaxCount(mData.RowCount() + 1);
          mData.EndRow();
        }
        cellsInRow = 0;
        skipRow = false;
        quoted = false;
      };

      while (p_FileLength > 0)
      {
        const std::streamsize toReadLength = std::min<std::streamsize>(p_FileLength, bufLength);
        pStream.read(buffer.data(), toReadLength);

        // With user-specified istream opened in non-binary mode on windows, we may have a
        // data length mismatch, so ensure we don't parse outside actual data length read.
        const std::streamsize readLength = pStream.gcount();
        if (readLength <= 0)
        {
          break;
        }

        for (size_t i = 0; i < static_cast<size_t>(readLength); ++i)
        {
          if (buffer[i] == mSeparatorParams.mQuoteChar)
          {
            if (cell.empty() || (cell[0] == mSeparatorParams.mQuoteChar))
            {
              quoted = !quoted;
            }
            else if (mSeparatorParams.mTrim)
            {
              // allow whitespace before first mQuoteChar
              const auto firstQuote = std::find(cell.begin(), cell.end(), mSeparatorParams.mQuoteChar);
              if (std::all_of(cell.begin(), firstQuote, [](unsigned char ch) { return isspace(ch); }))
              {
                quoted = !quoted;
              }
            }
            cell += buffer[i];
          }
          else if (buffer[i] == mSeparatorParams.mSeparator)
          {
            if (!quoted)
            {
              addCell();
            }
            else
            {
              cell += buffer[i];
            }
          }
          else if (buffer[i] == '\r')
          {
            if (mSeparatorParams.mQuotedLinebreaks && quoted)
            {
              cell += buffer[i];
            }
            else
            {
              ++cr;
            }
          }
          else if (buffer[i] == '\n')
          {
            if (mSeparatorParams.mQuotedLinebreaks && quoted)
            {
              cell += buffer[i];
            }
            else
            {
              // Without pQuotedLinebreaks a quoted field cannot span lines: a line break while
              // a quote is still open would otherwise end the record and silently drop the quote
              if (quoted)
              {
                const std::string errStr = "unterminated quoted field in record " +
                  std::to_string(mData.RowCount() + 1) + " (line break inside quotes requires pQuotedLinebreaks)";
                throw std::runtime_error(errStr);
              }

              ++lf;
              if (mLineReaderParams.mSkipEmptyLines && (cellsInRow == 0) && cell.empty())
              {
                // skip empty line
              }
              else
              {
                addCell();
                endRecord();
              }
            }
          }
          else
          {
            cell += buffer[i];
          }
        }
        p_FileLength -= readLength;
      }

      // An unterminated quoted field would otherwise swallow the rest of the input silently
      if (quoted)
      {
        const std::string errStr = "unterminated quoted field in record " + std::to_string(mData.RowCount() + 1);
        throw std::runtime_error(errStr);
      }

      // Handle last row / cell without linebreak
      if ((cellsInRow == 0) && cell.empty())
      {
        // skip empty trailing line
      }
      else
      {
        addCell();
        endRecord();
      }

      mData.ShrinkToFit();

      // Assume CR/LF if at least half the linebreaks have CR
      mSeparatorParams.mHasCR = (cr > (lf / 2));

      // Set up column labels
      UpdateColumnNames();
      UpdateRowNames();
    }

    void WriteCsv() const
    {
      if (mIsUtf16)
      {
        std::stringstream ss;
        WriteCsv(ss);
        const std::string utf16 = Utf8ToUtf16(ss.str(), mIsLE);

        std::ofstream stream;
        stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        stream.open(mPath, std::ios::binary | std::ios::trunc);
        stream.write(utf16.data(), static_cast<std::streamsize>(utf16.size()));
      }
      else
      {
        std::ofstream stream;
        stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        stream.open(mPath, std::ios::binary | std::ios::trunc);
        if (mHasUtf8BOM)
        {
          stream.write(s_Utf8BOM.data(), 3);
        }

        WriteCsv(stream);
      }
    }

    void WriteCsv(std::ostream& pStream) const
    {
      // One cell buffer is reused for the whole table: no per-row vector, no per-cell copy.
      std::string cell;
      const char* const eol = mSeparatorParams.mHasCR ? "\r\n" : "\n";
      for (size_t r = 0; r < mData.RowCount(); ++r)
      {
        const size_t width = mData.RowWidth(r);
        for (size_t c = 0; c < width; ++c)
        {
          mData.CopyCell(r, c, cell);
          if (mSeparatorParams.mAutoQuote &&
              ((cell.find(mSeparatorParams.mSeparator) != std::string::npos) ||
               (cell.find(mSeparatorParams.mQuoteChar) != std::string::npos) ||
               (cell.find_first_of(" \t\n\v\f\r") != std::string::npos)))
          {
            // escape quotes in string
            std::string str = cell;
            const std::string quoteCharStr = std::string(1, mSeparatorParams.mQuoteChar);
            ReplaceString(str, quoteCharStr, quoteCharStr + quoteCharStr);

            pStream << quoteCharStr << str << quoteCharStr;
          }
          else
          {
            pStream << cell;
          }

          if (c + 1 < width)
          {
            pStream << mSeparatorParams.mSeparator;
          }
        }
        pStream << eol;
      }
    }

    size_t GetDataRowCount() const
    {
      return mData.RowCount();
    }

    size_t GetDataColumnCount() const
    {
      const size_t firstDataRow =
        static_cast<size_t>((mLabelParams.mColumnNameIdx >= 0) ? mLabelParams.mColumnNameIdx : 0);
      return (mData.RowCount() > firstDataRow) ? mData.RowWidth(firstDataRow) : 0;
    }

    inline size_t GetDataRowIndex(const size_t pRowIdx) const
    {
      const size_t firstDataRow =
        static_cast<size_t>((mLabelParams.mColumnNameIdx + 1 >= 0) ? mLabelParams.mColumnNameIdx + 1 : 0);
      return pRowIdx + firstDataRow;
    }

    inline size_t GetDataColumnIndex(const size_t pColumnIdx) const
    {
      const size_t firstDataColumn =
        static_cast<size_t>((mLabelParams.mRowNameIdx + 1 >= 0) ? mLabelParams.mRowNameIdx + 1 : 0);
      return pColumnIdx + firstDataColumn;
    }

    /**
     * @brief   Get the raw string of a data cell, with bounds checking.
     * @details Reports the row and column indices passed by the caller, and the size of the
     *          row that was accessed, so that rows shorter than the header are diagnosable.
     * @param   pColumnIdx            zero-based column index, as passed by the caller.
     * @param   pRowIdx               zero-based row index, as passed by the caller.
     * @returns reference to the raw cell string.
     * @throws  std::out_of_range if the row or the column does not exist.
     */
    std::string GetCellValue(const size_t pColumnIdx, const size_t pRowIdx) const
    {
      std::string value;
      GetCellInto(pColumnIdx, pRowIdx, value);
      return value;
    }

    /**
     * @brief   Copies one cell into pValue, validating the indices once.
     * @param   pColumnIdx            column index, as seen by the user
     * @param   pRowIdx               row index, as seen by the user
     * @param   pValue                destination string, overwritten
     * @throws  std::out_of_range     if the row or column does not exist
     */
    void GetCellInto(const size_t pColumnIdx, const size_t pRowIdx, std::string& pValue) const
    {
      const size_t dataRowIdx = GetDataRowIndex(pRowIdx);
      if (dataRowIdx >= mData.RowCount())
      {
        const std::string errStr = "row index " + std::to_string(pRowIdx) +
          " out of range (rows: " + std::to_string(GetRowCount()) + ")";
        throw std::out_of_range(errStr);
      }

      const size_t width = mData.RowWidth(dataRowIdx);
      const size_t dataColumnIdx = GetDataColumnIndex(pColumnIdx);
      if (dataColumnIdx >= width)
      {
        const size_t firstDataColumn = GetDataColumnIndex(0);
        const size_t dataCells = (width > firstDataColumn) ? width - firstDataColumn : 0;
        const std::string errStr = "column index " + std::to_string(pColumnIdx) +
          " out of range for row " + std::to_string(pRowIdx) +
          " (row has " + std::to_string(dataCells) + " cells)";
        throw std::out_of_range(errStr);
      }

      mData.CopyCell(dataRowIdx, dataColumnIdx, pValue);
    }

    /**
     * @brief   GetCell() for std::string: copies the stored cell directly into the result.
     */
    void GetCellTo(const size_t pColumnIdx, const size_t pRowIdx, std::string& pVal,
                   std::true_type) const
    {
      GetCellInto(pColumnIdx, pRowIdx, pVal);
    }

    /**
     * @brief   GetCell() for any other type: converts the stored cell with Converter<T>.
     */
    template<typename T>
    void GetCellTo(const size_t pColumnIdx, const size_t pRowIdx, T& pVal, std::false_type) const
    {
      Converter<T> converter(mConverterParams);
      converter.ToVal(GetCellValue(pColumnIdx, pRowIdx), pVal);
    }

    /**
     * @brief   Trims and unquotes a raw cell, working in place on the moved-in string.
     * @details Replaces the former Trim()/Unquote() pair, which copied the string at each step.
     * @param   pStr                  raw cell content, taken by value so it can be moved from.
     * @returns cell content after trimming and quote removal.
     */
    std::string FinishCell(std::string pStr) const
    {
      if (mSeparatorParams.mTrim)
      {
        // ltrim
        pStr.erase(pStr.begin(), std::find_if(pStr.begin(), pStr.end(),
                                              [](unsigned char ch) { return !isspace(ch); }));

        // rtrim
        pStr.erase(std::find_if(pStr.rbegin(), pStr.rend(),
                                [](unsigned char ch) { return !isspace(ch); }).base(), pStr.end());
      }

      if (mSeparatorParams.mAutoQuote && (pStr.size() >= 2) &&
          (pStr.front() == mSeparatorParams.mQuoteChar) &&
          (pStr.back() == mSeparatorParams.mQuoteChar))
      {
        // remove start/end quotes
        pStr.pop_back();
        pStr.erase(pStr.begin());

        // unescape quotes in string
        const std::string quoteCharStr(1, mSeparatorParams.mQuoteChar);
        ReplaceString(pStr, quoteCharStr + quoteCharStr, quoteCharStr);
      }

      // cells are built one character at a time, so their capacity can exceed their size
      pStr.shrink_to_fit();
      return pStr;
    }

    void UpdateColumnNames()
    {
      mColumnNames.clear();
      if ((mLabelParams.mColumnNameIdx >= 0) &&
          (static_cast<std::ptrdiff_t>(mData.RowCount()) > mLabelParams.mColumnNameIdx))
      {
        const size_t nameRowIdx = static_cast<size_t>(mLabelParams.mColumnNameIdx);
        const size_t width = mData.RowWidth(nameRowIdx);
        for (size_t i = 0; i < width; ++i)
        {
          mColumnNames[mData.Get(nameRowIdx, i)] = i;
        }
      }
    }

    void UpdateRowNames()
    {
      mRowNames.clear();
      if ((mLabelParams.mRowNameIdx >= 0) &&
          (static_cast<std::ptrdiff_t>(mData.RowCount()) >
           (mLabelParams.mColumnNameIdx + 1)))
      {
        for (size_t r = 0; r < mData.RowCount(); ++r)
        {
          // rows too short to hold a row label are left unnamed, but must still be counted
          if (static_cast<std::ptrdiff_t>(mData.RowWidth(r)) > mLabelParams.mRowNameIdx)
          {
            mRowNames[mData.Get(r, static_cast<size_t>(mLabelParams.mRowNameIdx))] = r;
          }
        }
      }
    }

    // Unicode code point used in place of malformed input.
    static const uint32_t s_ReplacementChar = 0x0000fffd;

    // Reads the UTF-16 code unit at byte offset pIdx, which must be a valid offset
    // of a complete code unit.
    static uint32_t GetUtf16Unit(const char* pData, size_t pIdx, bool pIsLE)
    {
      const uint32_t byte0 = static_cast<unsigned char>(pData[pIdx]);
      const uint32_t byte1 = static_cast<unsigned char>(pData[pIdx + 1]);
      return pIsLE ? ((byte1 << 8) | byte0) : ((byte0 << 8) | byte1);
    }

    static void AppendUtf16Unit(uint32_t pUnit, bool pIsLE, std::string& pUtf16)
    {
      const char lowByte = static_cast<char>(pUnit & 0xff);
      const char highByte = static_cast<char>((pUnit >> 8) & 0xff);
      pUtf16 += pIsLE ? lowByte : highByte;
      pUtf16 += pIsLE ? highByte : lowByte;
    }

    static void AppendUtf16(uint32_t pCodePoint, bool pIsLE, std::string& pUtf16)
    {
      if (pCodePoint >= 0x10000)
      {
        // encode as surrogate pair
        const uint32_t offset = pCodePoint - 0x10000;
        AppendUtf16Unit(0xd800 + (offset >> 10), pIsLE, pUtf16);
        AppendUtf16Unit(0xdc00 + (offset & 0x3ff), pIsLE, pUtf16);
      }
      else
      {
        AppendUtf16Unit(pCodePoint, pIsLE, pUtf16);
      }
    }

    static void AppendUtf8(uint32_t pCodePoint, std::string& pUtf8)
    {
      if (pCodePoint < 0x80)
      {
        pUtf8 += static_cast<char>(pCodePoint);
      }
      else if (pCodePoint < 0x800)
      {
        pUtf8 += static_cast<char>(0xc0 | (pCodePoint >> 6));
        pUtf8 += static_cast<char>(0x80 | (pCodePoint & 0x3f));
      }
      else if (pCodePoint < 0x10000)
      {
        pUtf8 += static_cast<char>(0xe0 | (pCodePoint >> 12));
        pUtf8 += static_cast<char>(0x80 | ((pCodePoint >> 6) & 0x3f));
        pUtf8 += static_cast<char>(0x80 | (pCodePoint & 0x3f));
      }
      else
      {
        pUtf8 += static_cast<char>(0xf0 | (pCodePoint >> 18));
        pUtf8 += static_cast<char>(0x80 | ((pCodePoint >> 12) & 0x3f));
        pUtf8 += static_cast<char>(0x80 | ((pCodePoint >> 6) & 0x3f));
        pUtf8 += static_cast<char>(0x80 | (pCodePoint & 0x3f));
      }
    }

    // Converts UTF-16 encoded data (without byte order mark) to UTF-8. Surrogate pairs
    // are combined into a single code point. Unpaired surrogates and a trailing odd byte
    // are replaced with U+FFFD.
    static std::string Utf16ToUtf8(const char* pData, size_t pSize, bool pIsLE)
    {
      std::string utf8;
      utf8.reserve(pSize);

      size_t idx = 0;
      while ((idx + 1) < pSize)
      {
        uint32_t codePoint = GetUtf16Unit(pData, idx, pIsLE);
        idx += 2;

        if ((codePoint >= 0xd800) && (codePoint <= 0xdbff) && ((idx + 1) < pSize))
        {
          // high surrogate - combine with low surrogate, when present
          const uint32_t lowSurrogate = GetUtf16Unit(pData, idx, pIsLE);
          if ((lowSurrogate >= 0xdc00) && (lowSurrogate <= 0xdfff))
          {
            codePoint = 0x10000 + ((codePoint - 0xd800) << 10) + (lowSurrogate - 0xdc00);
            idx += 2;
          }
        }

        if ((codePoint >= 0xd800) && (codePoint <= 0xdfff))
        {
          // unpaired surrogate
          codePoint = s_ReplacementChar;
        }

        AppendUtf8(codePoint, utf8);
      }

      if (idx < pSize)
      {
        // trailing odd byte
        AppendUtf8(s_ReplacementChar, utf8);
      }

      return utf8;
    }

    // Converts UTF-8 encoded data to UTF-16, prefixed with a byte order mark. Code points
    // outside the basic multilingual plane are encoded as surrogate pairs. Malformed input
    // is replaced with U+FFFD.
    static std::string Utf8ToUtf16(const std::string& pUtf8, bool pIsLE)
    {
      std::string utf16;
      utf16.reserve((pUtf8.size() + 1) * 2);

      AppendUtf16(0x0000feff, pIsLE, utf16);

      size_t idx = 0;
      while (idx < pUtf8.size())
      {
        const uint32_t leadByte = static_cast<unsigned char>(pUtf8[idx]);
        uint32_t codePoint = 0;
        size_t seqLen = 0;

        if (leadByte < 0x80)
        {
          codePoint = leadByte;
          seqLen = 1;
        }
        else if ((leadByte & 0xe0) == 0xc0)
        {
          codePoint = leadByte & 0x1f;
          seqLen = 2;
        }
        else if ((leadByte & 0xf0) == 0xe0)
        {
          codePoint = leadByte & 0x0f;
          seqLen = 3;
        }
        else if ((leadByte & 0xf8) == 0xf0)
        {
          codePoint = leadByte & 0x07;
          seqLen = 4;
        }
        else
        {
          // invalid lead byte
          codePoint = s_ReplacementChar;
          seqLen = 1;
        }

        if ((seqLen > 1) && ((idx + seqLen) <= pUtf8.size()))
        {
          for (size_t byteIdx = 1; byteIdx < seqLen; ++byteIdx)
          {
            const uint32_t contByte = static_cast<unsigned char>(pUtf8[idx + byteIdx]);
            if ((contByte & 0xc0) != 0x80)
            {
              // invalid continuation byte
              codePoint = s_ReplacementChar;
              seqLen = 1;
              break;
            }

            codePoint = (codePoint << 6) | (contByte & 0x3f);
          }
        }
        else if (seqLen > 1)
        {
          // truncated sequence
          codePoint = s_ReplacementChar;
          seqLen = 1;
        }

        // reject out of range code points, surrogates and overlong sequences
        static const uint32_t minCodePoint[5] = { 0, 0, 0x80, 0x800, 0x10000 };
        if ((codePoint > 0x0010ffff) ||
            ((codePoint >= 0xd800) && (codePoint <= 0xdfff)) ||
            (codePoint < minCodePoint[seqLen]))
        {
          codePoint = s_ReplacementChar;
        }

        AppendUtf16(codePoint, pIsLE, utf16);
        idx += seqLen;
      }

      return utf16;
    }

    static void ReplaceString(std::string& pStr, const std::string& pSearch, const std::string& pReplace)
    {
      size_t pos = 0;

      while ((pos = pStr.find(pSearch, pos)) != std::string::npos)
      {
        pStr.replace(pos, pSearch.size(), pReplace);
        pos += pReplace.size();
      }
    }

  private:
    std::string mPath;
    LabelParams mLabelParams;
    SeparatorParams mSeparatorParams;
    ConverterParams mConverterParams;
    LineReaderParams mLineReaderParams;
    CellTable mData;
    std::map<std::string, size_t> mColumnNames;
    std::map<std::string, size_t> mRowNames;
    bool mIsUtf16 = false;
    bool mIsLE = false;
    bool mHasUtf8BOM = false;
  };
}
