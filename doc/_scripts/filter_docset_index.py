#!/usr/bin/env python3

import sqlite3

db_conn = sqlite3.connect("../docSet.dsidx")
cur = db_conn.cursor()
cur2 = db_conn.cursor()
res = cur.execute("select * from searchIndex")

for id, name, t, path in res:
    if name.find("__") != -1 and any([name.startswith(x) for x in ["group", "union", "struct"]]):
        cur2.execute(f"delete from searchIndex where id={id}")

db_conn.commit()
