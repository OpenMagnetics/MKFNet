%module MKFNetModule


%include <std_string.i>
%include <std_vector.i>
%include <typemaps.i>
%apply const std::string & {std::string &};

%{
  #include "MKFNet.h"
%}


%include "MKFNet.h"