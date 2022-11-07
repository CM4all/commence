c = mariadb:new({host="192.168.33.6",user='foo',passwd='bar',db='xyz'})
result = c:execute("SELECT NOW()")
row = result:fetch()
print("now", row["NOW()"])
