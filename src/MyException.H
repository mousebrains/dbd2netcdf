#ifndef INC_MyException_H_
#define INC_MyException_H_

#include <exception>
#include <string>

class MyException : public std::exception {
private:
  const std::string mWhat;
public:
  MyException(const std::string& msg) : mWhat(msg) {}
  virtual ~MyException() noexcept {}

  virtual const char* what() const noexcept {return mWhat.c_str();}
}; // MyException

#endif // INC_MyException_H_

/*
    This file is part of dbd2netCDF.

    dbd2netCDF is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    dbd2netCDF is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with dbd2netCDF.  If not, see <http://www.gnu.org/licenses/>.
*/
