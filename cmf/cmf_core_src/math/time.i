﻿%{
#include "datetime.h"
#include "math/real.h"
#include "math/Time.h"
#include "math/timeseries.h"
%}
// Get Math
%pythoncode
{
import datetime
import struct
}
%init %{
PyDateTime_IMPORT;
%}

%{
static cmf::math::Time convert_datetime_to_cmftime(PyObject* dt)
{
    void * pt; 
    int res=SWIG_ConvertPtr(dt,&pt,SWIGTYPE_p_cmf__math__Time,0);
    if (SWIG_IsOK(res))
    {
        cmf::math::Time * temp = reinterpret_cast< cmf::math::Time * >(pt);
        return *temp;
    }
    else if (PyDateTime_Check(dt))
       return cmf::math::Time(PyDateTime_GET_DAY(dt),
                              PyDateTime_GET_MONTH(dt),
                              PyDateTime_GET_YEAR(dt),
                              PyDateTime_TIME_GET_HOUR(dt),
                              PyDateTime_TIME_GET_MINUTE(dt),
                              PyDateTime_TIME_GET_SECOND(dt),
                              PyDateTime_TIME_GET_MICROSECOND(dt)/1000);
   else if (PyDate_Check(dt))
       return cmf::math::Time(PyDateTime_GET_DAY(dt),
                              PyDateTime_GET_MONTH(dt),
                              PyDateTime_GET_YEAR(dt));
   else if (PyDelta_Check(dt))
   {
     PyDateTime_Delta* delta=(PyDateTime_Delta*)(dt);
     long long ms=24 * 3600;
     ms*=delta->days;
     ms+=delta->seconds;
     ms*=1000;
     ms+=delta->microseconds/1000;
     return cmf::math::Time::Milliseconds(ms);
   }
   else 
   {
     PyErr_SetString(PyExc_ValueError,"Type is neither a cmf.Time nor a Python datetime object");
     return cmf::math::Time();
   } 
}
%}                         

%typemap(in) cmf::math::Time {
    $1 = convert_datetime_to_cmftime($input);    
}
%typemap(typecheck,precedence=0) cmf::math::Time {
    void * pt;    
    int res=SWIG_ConvertPtr($input,&pt,SWIGTYPE_p_cmf__math__Time,0);
    $1=SWIG_IsOK(res) || PyDateTime_Check($input) || PyDelta_Check($input) || PyDate_Check($input);
}
%implicitconv cmf::math::Time;
%implicitconv cmf::math::Date;

%include "math/real.h"
%include "math/Time.h"

//%naturalvar cmf::math::Time;

%attributeval(cmf::math::timeseries,cmf::math::Time,begin,begin);
%attributeval(cmf::math::timeseries,cmf::math::Time,step,step);
%attributeval(cmf::math::timeseries,cmf::math::Time,end,end);
//%naturalvar cmf::math::timeseries;

%implicitconv cmf::math::timeseries;

%include "math/timeseries.h"






%extend cmf::math::Time {
    %pythoncode 
    {
    def __repr__(self):
        if self>year*40:
            return self.AsDate().to_string()
        else:
            return self.to_string()
    def __nonzero__(self):
        return self.is_not_0();
    def __rmul__(self,other):
        return self*other;
    def AsPython(self):
        d=self.AsDate()
        return datetime.datetime(d.year,d.month,d.day,d.hour,d.minute,d.second,d.ms*1000)
    }
}

%extend cmf::math::Date {
    %pythoncode 
    {
    def __repr__(self):
        return self.to_string()
    def AsPython(self):
        return datetime.datetime(self.year,self.month,self.day,self.hour,self.minute,self.second,self.ms*1000)
    }
}



%extend cmf::math::timeseries
{
	double __len__()
	{
		return $self->size();
	}
	%pythoncode
    {
    def __repr__(self):
       return "cmf.timeseries(%s:%s:%s,count=%i)" % (self.begin,self.end,self.step,self.size())
    def extend(self,list) :
        """ Adds the values of a sequence to the timeseries"""
        for item in list :
            self.add(float(item))
    def __getitem__(self,index):
        if isinstance(index,int):
            return self.get_i(index)
        elif isinstance(index,slice):
            if index.step:
                return self.get_slice(index.start,index.stop,index.step)
            else:
                return self.get_slice(index.start,index.stop)
        else:
            return self.get_t(index)
    def __setitem__(self,index,value):
        if isinstance(index,int):
            self.set_i(index,value)
        if isinstance(index,slice):
            if index.step:
                raise ValueError("Slices must be continous, when used for setting")
            else:
                if not isinstance(value,timeseries):
                    value=timeseries(value)
                self.set_slice(index.start,index.stop,value)
        else:
            self.set_t(index,value)
    def __iter__(self):
        for i in xrange(self.size()):
            yield self.get_i(i)
    def interpolate(self,begin,end,step):
        """ Returns a generator returning the interpolated values at the timesteps """
        if step>self.step():
            ts=self.reduce_avg(begin,step)
        else:
            ts=self
        for t in timerange(step,end,step):
            yield ts[t]
    def __radd__(self,other):
        return self + other;
    def __rmul__(self,other):
        return self + other;
    def __rsub__(self,other):
        res=-self
        res+=other
        return res
    def __rdiv__(self,other):
        res=self.inv() 
        res*=other
        return res
    def iter_time(self, as_float=0):
        """Returns an iterator to iterate over each timestep
        as_float if True, the timesteps will returned as floating point numbers representing the days after 1.1.0001 00:00
        """
        for i in xrange(len(self)):
            if as_float:
                yield ((self.begin + self.step * i) - cmf.Time(1,1,1)).AsDays()
            else:
                yield self.begin + self.step * i
    def to_buffer(self):
        """Returns a binary buffer filled with the data of self"""
        return struct.pack('qqqq%id' % self.size(),self.begin.AsMilliseconds(),self.step.AsMilliseconds(),self.interpolationpower(), *self)
    def to_file(self,f):
        """ Saves a timeseries in a special binary format.
        The format consists of 4 integers with 64 bit, indicating the milliseconds after the 31.12.1899 00:00 of the beginning of the timeseries, the milliseconds of the time step,
        the interpolation power and the number of values. The following 64 bit floats, are the values of the timeseries
        """
        if isinstance(f,str):
            f=file(f,'wb')
        elif not hasattr(f,'write'):
            raise TypeError("The file f must be either an object providing a write method, like a file, or a valid file name")
        f.write(struct.pack('qqqq%id' % self.size(),  self.size(), self.begin.AsMilliseconds(),self.step.AsMilliseconds(),self.interpolationpower(), *self))
        
    @classmethod
    def from_sequence(cls,begin,step,sequence=[],interpolation_mode=1):
        res=cmf.timeseries(begin,step,interpolation_mode)
        res.extend(sequence)
        
    @classmethod
    def from_buffer(cls,buf):
        header_length=struct.calcsize('qqqq') 
        header=struct.unpack('qqqq',buffer[:header_length])
        res=cls(header[1]*ms,header[2]*ms,header[3])
        res.extend(struct.unpack('%id' % header[0],*buffer(buf,header_length,header[0]*8)))
    @classmethod
    def from_file(cls,f):
        """ Loads a timeseries saved with to_file from a file 
        Description of the file layout:
        byte: 
        0   Number of (int64)
        8   Begin of timeseries (in ms since 31.12.1899 00:00) (int64)
        16  Step size of timeseries (in ms) (int64)
        24  Interpolation power (int64)
        32  First value of timeseries (float64)
        """
        if isinstance(f,str):
            f=file(f,'rb')
        elif not hasattr(f,'read'):
            raise TypeError("The file f must either implement a 'read' method, like a file, or must be a vild file name")
        header_length=struct.calcsize('qqqq') 
        header=struct.unpack('qqqq',f.read(header_length))
        res=cls(header[1]*ms,header[2]*ms,header[3])
        res.extend(struct.unpack('%id' % header[0],f.read(-1)))
        return res
    }
}
        

%pythoncode {
def AsCMFtime(date):
    """Converts a python datetime to cmf.Time"""
    return Time(date.day,date.month,date.year,date.hour,date.minute,date.second,date.microsecond/1000)
def timerange(start,end,step=day):
    """Creates a generator of cmf.Time, similar to the Python range function"""
    return [start+step*x for x in range(0,int((end-start)/step))]
def xtimerange(start,end,step=day):
    """Creates a generator of cmf.Time, similar to the Python range function"""
    return (start+step*x for x in range(0,int((end-start)/step)))
}