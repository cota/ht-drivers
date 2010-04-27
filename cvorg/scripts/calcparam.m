p = 16;
b = 4218;
% N = p * b
a = 12;

freq = 10e6;
fvco = 2.7e9;

dv = 1;
d1 = 1;
d2 = 1;
mindiff = 100e9;
elem = 0;

% first make an accurate approximation through the dividers
for d1=1:32
        diff = fvco / dv / d1 / d2 - freq;
        if abs(diff) < abs(mindiff) && diff >= 0
                mindiff = diff;
                elem = d1;
        end
end

d1 = elem
mindiff

for d2=1:32
        diff = fvco / dv / d1 / d2 - freq;
        if abs(diff) < abs(mindiff) && diff >= 0
                mindiff = diff;
                elem = d2;
        end
end

d2 = elem
mindiff

for dv=1:6
	diff = fvco / dv / d1 / d2 - freq;
	if abs(diff) < abs(mindiff)
		mindiff = diff;
		elem = dv;
	end
end

dv = elem
mindiff

freq
mindiff + freq
mindiff
