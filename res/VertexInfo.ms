ConvertToMesh objects
f = createFile "vertex.txt"
select $*
objs = $
for o in objs do
(
	if classof o != Editable_mesh then continue
	local count
	count = o.numverts
	if count == 0 then continue
	format "\n{NAME} : {%}\n" o.name to:f
	format "{COUNT} : {%}\n" count to:f
	for k = 1 to count do
	(
		pt = in coordsys world getVert o k
		format "{%} : {%}\n" k pt to:f
	)
)