recursive_delete(path/"foo")
make_directory(path/"foo/a/b")
recursive_copy(src/"../src", path/"foo/a/b");

name="world"
r=Random:new("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_ []{}<>~`+=,.;:/?|")
copy_template(src/"template.txt", path/"with_template")
