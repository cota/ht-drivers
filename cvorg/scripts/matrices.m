z = 5
V = []';

for i = 1 : z
	for j = i : z
		A(i,j) = i * j;
		V = [V; A(i,j)];
	end
end

A
uniq = create_set(V)
elems = length(uniq)
