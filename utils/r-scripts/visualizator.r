library("int64")

args <- commandArgs(trailingOnly = T)
file = args[1]

# assume that file is in binary format
table <- read.table(file, header=TRUE)

# INFOCOLL_BEGIN  0
# INFOCOLL_END    1
# INFOCOLL_OPEN   2
# INFOCOLL_READ   3
# INFOCOLL_WRITE  4
# INFOCOLL_LSEEK  5
# INFOCOLL_FSYNC  6
# INFOCOLL_MKNOD  7
# INFOCOLL_UNLINK 8
# INFOCOLL_ALLOCATE 9
# INFOCOLL_TRUNCATE 10
# INFOCOLL_CLOSE  11

table <- subset(table, type == 3 | type == 4) # || type == 2 || type == 11)

colors <- c('white', 'white', 'green',
			'blue', 'red', 'white',
			'white', 'white', 'white',
			'white', 'white', 'black')

svg("plot.svg")
plot(table$time, table$info_3, col=colors[table$type], xlim=c(min(table$time), max(table$time)))
dev.off()
