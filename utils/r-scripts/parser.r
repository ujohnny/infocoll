library("int64")

args <- commandArgs(trailingOnly = T)
file = args[1]

# assume that file is in binary format
fd <- file(description=file, open="rb", raw=TRUE)

rawToInt64 <- function(v) Reduce((function(a, x) a * 256L + x ), v, int64(1))
int64ToDouble <- function (v) mapply(function (a) as.double(a[[1]]) * 2^32 + ((as.double(a[[2]]) + 2^32) %% 2^32), v)

# table <- merge(data.frame(), data.frame(type=0, time=0, f1=0, f2=0, f3=0, f4=0, f5=0))
table <- NULL

while (length(buffer <- readBin(fd, "raw", 57))) {
	buffer <- as.integer(buffer)

	type <- head(buffer, 1)
	buffer <- tail(buffer, -1)

	time_sec <- int64ToDouble(rawToInt64(head(buffer, 8)))
	buffer <- tail(buffer, -8)

	time_nsec <- int64ToDouble(rawToInt64(head(buffer, 8)))
	buffer <- tail(buffer, -8)

	time <- time_sec + time_nsec * 1e-9

	f1 <- rawToInt64(head(buffer, 8))
	buffer <- tail(buffer, -8)

	f2 <- rawToInt64(head(buffer, 8))
	buffer <- tail(buffer, -8)

	f3 <- rawToInt64(head(buffer, 8))
	buffer <- tail(buffer, -8)

	f4 <- rawToInt64(head(buffer, 8))
	buffer <- tail(buffer, -8)

	f5 <- rawToInt64(head(buffer, 8))
	buffer <- tail(buffer, -8)

	rbind(table, data.frame(type=type, time=time, f1=f1, f2=f2, f3=f3, f4=f4, f5=f5)) -> table
#	table <- merge(table, data.frame(type=type, time=time, f1=f1, f2=f2, f3=f3, f4=f4, f5=f5), all=TRUE)
	print(c(time))
}

print(table)
