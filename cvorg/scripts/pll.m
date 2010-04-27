fref = 2.7e9;
a=[2:6]';
%b=[1:16 17 19 23 29 31]';
b=[1:32]';

res = []';

outfile = "pllplot.dat";

fd = fopen(outfile, "wt");

fprintf(fd, "Output\t       A[1,6]\t   B[1,32]\t   C[1,32]\n");
fprintf(fd, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

for A=a(1):a(end)
	for B=b(1):b(end)
		for C=B:b(end)
			res = [res; fref/(A*B*C), A, B, C];
			fprintf(fd, "%11d\t      %1d\t       %2d\t       %2d\n", fref/(A*B*C), A, B, C);
		end
	end
end

fclose(fd);

nrrep = length(res(:,1));
result = create_set(res(:,1));
nrnorep = length(result);

printf("Copied %d points to file %s, %d points repeated, %d unique values\n",
	nrrep, outfile, nrrep - nrnorep, nrnorep);
