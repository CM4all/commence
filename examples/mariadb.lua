c = mariadb:new({host="192.168.33.6",user='foo',passwd='bar',db='xyz',multi_statements=true})
result = c:execute("SELECT NOW(); SELECT 42;")
row = result:fetch()
print("now", row["NOW()"])
