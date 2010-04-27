b=[1:5]';

res = []';

outfile = "numbers.dat"

fd = fopen(outfile, "wt");

%fprintf(fd, "Result\t       B[1,32]\t   C[1,32]\n");
%fprintf(fd, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
%for B=b(1):b(end)
for B=b(1):b(end)
	%for C=b(1):b(end)
	for C=b(1):b(end)
		res = [res; B*C, B, C];
		fprintf(fd, "%11d\t      %2d\t       %2d\n", B*C, B, C);
	endfor
endfor

fclose(fd);

nrrep = length(res(:,1));
result = create_set(res(:,1));
nrnorep = length(result);

printf("Copied %d points to file %s, %d points repeated, %d unique values\n",
	nrrep, outfile, nrrep - nrnorep, nrnorep);
