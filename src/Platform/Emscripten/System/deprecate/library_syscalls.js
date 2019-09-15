'use strict'

var funs = {
  // syscalls implementation
  syscalljs_{{{ cDefine('SYS_sigreturn') }}}: function() {
    Module.printErr('SYS_sigreturn STUB implementation');
    return 0;
  },
  syscalljs_{{{ cDefine('SYS_rt_sigreturn') }}}: function() {
    Module.printErr('SYS_rt_sigreturn STUB implementation');
    return 0;
  }
};

mergeInto(LibraryManager.library, funs);

