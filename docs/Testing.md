# Tests
[test.sh](https://github.com/fennecdjay/Gwion/blob/dev/util/test.sh)
requires [valgrind](http://valgrind.org/) 
there are two kinds of tests:
  * [gwion](#gwion-tests)
  * [bash](#bash-tests)
  
## Gwion tests
those tests are just gwion (.gw) files, handling special comments:
  * `// [skip]`     (*optionally* followed by reason to skip)
  * `// [todo]`     (*optionally* followed by reason to delay testing)
  * `// [contains]` followed by string to match
  * `// [excludes]` followed by string not to match

## Shell test
those tests are just bash (.sh) files.  
they should start with this snippet
```bash
#!/bin/bash
# [test] #5
n=0
[ "$1" ] && n="$1"
[ "$n" -eq 0 ] && n=1
source tests/sh/common.sh
```

## TODO
  [ ] `bailout` system for early exit on failure
