library("int64")

args <- commandArgs(trailingOnly = T)
file = args[1]
axisX = args[2]
axisY = args[3]
fileActionLines = as.logical(args[4])

# assume that file is in R table format
table <- read.table(file, header=TRUE)

# INFOCOLL_BEGIN  0
# INFOCOLL_END    1
# INFOCOLL_OPEN   2
#
# INFOCOLL_READ   3
# INFOCOLL_WRITE  4
# INFOCOLL_LSEEK  5
# 
# INFOCOLL_FSYNC  6
# INFOCOLL_MKNOD  7
# INFOCOLL_UNLINK 8
# 
# INFOCOLL_ALLOCATE 9
# INFOCOLL_TRUNCATE 10
# INFOCOLL_CLOSE  11

openclose <- subset(table, type == 2 | type == 11)
readwrite <- subset(table, type == 3 | type == 4)
inodes <- unique(subset(table, type != 0)$inode)

colors <- c('green', 'blue', 'red', 'orange', 'black', 'purple', 'grey')
types <- c(0, 0, 0,
           1, 4, 0,
           0, 0, 0,
           0, 0, 0)
lty <- c(0, 0, 1,
         0, 0, 0,
         0, 0, 0,
         0, 0, 2)

svg(paste(basename(file), paste(axisX, axisY, sep="-"), "svg",sep="."))
layout(matrix(c(1,2), nrow = 1), widths = c(1, 0.3))

xargs <- as.list(readwrite[axisX])[[1]]
yargs <- as.list(readwrite[axisY])[[1]]

minX <- min(as.list(table[axisX])[[1]])
maxX <- max(as.list(table[axisX])[[1]])

plot(xargs, yargs,
     pch=types[readwrite$type + 1],
     col=colors[readwrite$inode %% length(colors) + 1],
     xlim=c(minX, maxX),
     xlab = axisX, ylab = axisY)
if (fileActionLines && args[2] == "time") {
  abline(v=openclose$time, col=colors[openclose$inode %% length(colors) + 1], lty=lty[openclose$type + 1], lwd=2)
}

par(mar = c(5, 0, 4, 2) + 0.1)
plot(1:3, rnorm(3), pch = 1, lty = 1, ylim=c(-2,2), type = "n", axes = FALSE, ann = FALSE)

legend(1, 1, paste("File", inodes), text.col=colors[inodes %% length(colors) + 1], pch=18)
legend(1, 2, c("Read", "Write"), pch=types[1 + c(3, 4)])
legend(1, -1, c("Open", "Close"), lty=lty[1+c(2, 11)])

dev.off()
