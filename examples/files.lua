recursive_delete(path/"foo")
d=make_directory(path/"foo/a/b")
recursive_copy(src/"../src", d)

name="world"
r=Random:new("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_ []{}<>~`+=,.;:/?|")
copy_template(src/"template.txt", path/"with_template")
