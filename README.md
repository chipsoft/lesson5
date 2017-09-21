Linux kernel module, that works with ranges in RB-tree

Here is output of working module:

root@qemux86:~# insmod home5.ko && rmmod home5.ko
[  650.068250] Our ranges:
[  650.070017] #1 - [0..5]
[  650.073948] #2 - [1..7]
[  650.077254] #3 - [2..9]
[  650.080088] #4 - [10..15]
[  650.086156] #5 - [20..25]
[  650.086156] 
[  650.093849] 
[  650.093849] After add first 3 ranges: 
[  650.105183] val(df648990): [0 - 9]
[  650.111407] 
[  650.111407] After del range #3
[  650.116569] val(df648990): [0 - 9]
[  650.120748] 
[  650.120748] After add range #3 again
[  650.127149] val(df648990): [0 - 9]
[  650.134020] 
[  650.134020] Result tree after all add:
[  650.146449] val(df548ff0): [20 - 25]
[  650.152576] val(df648990): [0 - 15]
[  650.158089] 
[  650.158089] Find range, that contain 11: [0 - 15]
[  650.169020] 
[  650.169020] That's all, folks!
[  650.169020]  

