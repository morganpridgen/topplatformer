import json, glob, sys

for path in glob.glob("%s/*.json" % sys.argv[1]):
  print(path)
  with open(path, "r") as f: data = json.loads(f.read())
  newPath = list(path)
  for i in range(5): newPath.pop(len(newPath) - 1)
  for i in ".lvl": newPath.append(i)
  out = []
  for i in "CM": out.append(ord(i))
  for i in range(8): out.append(i)
  for i in data["startpos"]: out.append(int(i))
  for i in range(45):
    for j in range(80):
      out.append(int(data["layout"][i][j]))
  out.append(0)
  with open("".join(newPath), "wb") as f:
    f.write(bytes(out))