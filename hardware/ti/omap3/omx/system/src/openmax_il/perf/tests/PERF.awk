# This script filters the PERF traces based on the specified parameters and
# measures the rate of the filtered traces.  This can be used to measure
# video encode/decode/preview frame rates.
# 
# Fields in a PERF CSV line:
#
# $1 - time stamp
# $2 - PID
# $3 - address
# $4 - component
# $5-$10 - domains (AD, VD, ID, AE, VE, IE)
# $11 - component type
# $12 - operation
# $13-... operation specific arguments:
#    Buffer:
# $13 - sending|received|xfering
# $14 - frame|buffer
# $15 - from
# $16 - to
# $17 - size
# $18 - address 1
# $19 - address 2
#    Boundary:
# $13 - hex
# $14 - textual

# initialize variables
BEGIN {
   FS = ",";                         # reading a CSV file

   what = what ? what : "frame";     # what buffers are we looking at
   how  = how  ? how  : "sending";   # what is the operation
   to   = to   ? to   : "";          # who are the recipients of these buffers
   from = from ? from : "";          # who are the senders of these buffers

   # boundary - only log buffer traces in the steady state of this component
   boundary = boundary ? boundary : "****";

   min  = (size != "") ? (size) : (min != "") ? (min) : 1;   # min size of buffers to watch
   max  = (size != "") ? (size) : (max != "") ? (max) : 5e9; # max size of buffers to watch
   # Additional variables not set:

   # debug    - debug flag
   # after    - only measure frames after the specified #
   # who      - who is logging these buffer transfers

   # get variable assignments from ARGV
   for (i=1; i<ARGC; i++) {
      arg = ARGV[i];
      if      (gsub("^what=",    "",arg)) { what     = arg }
      else if (gsub("^to=",      "",arg)) { to       = arg }
      else if (gsub("^from=",    "",arg)) { from     = arg }
      else if (gsub("^how=",     "",arg)) { how      = arg }
      else if (gsub("^who=",     "",arg)) { who      = arg }
      else if (gsub("^boundary=","",arg)) { boundary = arg }
      else if (gsub("^after=",   "",arg)) { after    = arg }
      else if (gsub("^debug=",   "",arg)) { debug    = arg }
      else if (gsub("^size=",    "",arg)) { min = max = (arg) }
      else if (gsub("^min=",     "",arg)) { min      = (arg) }
      else if (gsub("^max=",     "",arg)) { max      = (arg) }

      else continue;
      delete ARGV[i];
   }

   if (!who) {
      print "Must specify component to observe";
      exit 1;
   }

   # we are using the component thread as boundary by default
   if (boundary == "****") {
      if (substr(who, 0, 3) == "VP_" ||
          substr(who, 0, 3) == "VD_" ||
          substr(who, 0, 3) == "VE_" ||
          substr(who, 0, 3) == "CAM") {
         boundary = substr(who, 0, 3) "T";
      }
   }

   after++;   # we always have to after the 1st time stamp to get a time delta
   skip = after + 1;

   # start counting unless boundary is set, in which case
   count = boundary ? skip : 0;

   # initialize counters
   x = xx = N = 0;
   x_no_pause = xx_no_pause = N_no_pause = 0;

   if (debug > 1) {
      print "who = ", who
      print "how = ", how
      print "what = ", what
      print "from = ", (from ? from : "UNDEFINED")
      print "to = ", (to ? to : "UNDEFINED")
      print "min = ", min
      print "max = ", max
   }

   # convert to decimal
   min = min + 0
   max = max + 0
}

# Check for non-CSV trace file
/^</ {
   print "ERROR: non-CSV file encountered.  Please use csv = 1 in ./perf.ini";
   exit;
}

# Count frames
#   frames start with a number, with operation "Buffer"
/^[0-9]/ && $4 == who && $12 == "Buffer" &&
#   how and what has to match
#   if from or to are specified they also have to match
$13 == how && $14 == what && (!from || ($15 == from)) && (!to || ($16 == to)) &&
#   size has to fall in the range specified, we have to add 0 because of a
#   rare rounding issue in AWK when comparing hex and decimal numbers
((0 + $17) >= min) &&
((0 + $17) <= max) {
   # debug
   if (debug) { print $0 }

   # increase the count from the boundary
   if (count == after) {
      delta = $1 - last;
      if (delta >= 2) {
         print "Warning: Found a pause of", delta, "seconds";
      }
      else
      {
         x_no_pause  += delta;
         xx_no_pause += delta * delta;
         N_no_pause++;
      }
      x  += delta;
      xx += delta * delta;
      N++;
   }
   if (count < after) { count++; }
   last = $1;
}

# Check boundaries
/Steady/ && $12 == "Boundary" && $4 == boundary {
   # debug
   if (debug) { print $0 }

   # start counting if starting steady state, skip counting if ending steady
   # state
   count = /started/ ? 0 : skip;
}

END {
   # calculate inverse (1/x) average and variance
   if (N) {
      x /= N;
      xx /= N;
      s = xx ? (x * x / xx) : 0;
      result = 1/x " fps (s=" s ") (from " N " data points)";
      print "Rate is", result;

      if (N != N_no_pause) {
         if (N_no_pause) {
            x = x_no_pause / N_no_pause;
            xx = xx_no_pause / N_no_pause;
            s = xx ? (x * x / xx) : 1;
            result = 1/x " fps (s=" s ") (from " N " data points)";
            print "(Adjusted rate without pauses is", result, ")";
         } else {
            print "(Not enough data to calculate adjusted rate without pauses)";
         }
      }
   } else {
      print "Error:   Not enough data to calculate rate";
   }
}

