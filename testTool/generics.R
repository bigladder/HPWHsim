# Stupid stuff for the generic specifications...
library(EcotopePackage)
library(foreign)
library(ggplot2)
library(lme4)
library(Rcpp)
library(plyr)

# Get the COP at 67 for EVERYBODY!!!
cdx("hpwhlab")
files <- dir("../data/prepped")
files <- files[grep("67.dta", files)]


dset <- do.call('rbind', lapply(files, function(file) {
  tmp <- read.dta(paste("../data/prepped", file, sep = "/"))
  # Grab only times when it has been running...
  tmp$Tx <- (tmp$T_Tank_02 + tmp$T_Tank_04) / 2
  tmp <- tmp[, c("Tx", "COP5min")]
  tmp <- tmp[!is.na(tmp$COP5min), ]
  tmp$file <- file
  tmp
}))

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")

dset$make <- gsub("^.+_(.+)_.+$", "\\1", dset$file)
table(dset$make)
dset$Tx <- dset$Tx * 1.8 + 32

# Plot just the GE2014 and the HPTU and the Stiebel???
models2 <- c("GE502014", "AOSmithHPTU50", "SandenGAU")

allCop67v2 <- ggplot(dset[dset$make %in% models2, ]) + theme_bw() + 
  geom_point(aes(Tx, COP5min, col = make), size = 1, alpha = .5) +
  geom_smooth(aes(Tx, COP5min, col = make), method = "lm", se =FALSE) +
  ggtitle("Lab Test COP at 67F Air Temp\nAll Makes All Tests") +
  xlab("Temp at Bottom Third of Tank (F)") + ylab("5-Minute Estimated COP")
allCop67v2
# ggsave(file = "graphs/allCop67nodaikin.png", allCop67, width = 7.5, height = 5.5)


# Makes to remove...
toRemove <- c("ATI66", "Rheem", "SandenDR40gal", "SandenDR80gal", "SandenGAU", "SandenGES", "ATI50", "Mitsubishi", "Daikin")
dset <- dset[!(dset$make %in% toRemove), ]
dset <- dset[dset$Tx > 60, ]

allCop67 <- ggplot(dset) + theme_bw() + 
  geom_point(aes(Tx, COP5min, col = make), size = 1, alpha = .5) +
  geom_smooth(aes(Tx, COP5min, col = make), method = "lm", se =FALSE) +
  ggtitle("Lab Test COP at 67F Air Temp\nAll Makes All Tests") +
  xlab("Temp at Bottom Third of Tank (F)") + ylab("5-Minute Estimated COP")
allCop67
ggsave(file = "graphs/allCop67nodaikin.png", allCop67, width = 7.5, height = 5.5)





summary(lm(COP5min ~ Tx * make, data = dset))

mod <- lmer(COP5min ~  (1 | make) + Tx + (Tx | make), data = dset)
summary(mod)


# slope <- coef(mod <- lm(COP5min ~ Tx, dset))[2]
# intercept <- coef(mod)[1]
intercept <- fixef(mod)[1]
slope <- fixef(mod)[2]
delta <- 1.6
slopedelta <- 0.01
overall <- -.4
# slope <- -0.019
newDf1 <- data.frame("Tx" = seq(65, 130))
newDf1$fitted <- intercept - delta + overall + 0.3 + (slope + slopedelta) * newDf1$Tx
newDf1$Type <- "Generic 1"
newDf2 <- data.frame("Tx" = seq(65, 130))
newDf2$fitted <- intercept + overall + slope * newDf2$Tx 
newDf2$Type <- "Generic 2"
newDf3 <- data.frame("Tx" = seq(65, 130))
newDf3$fitted <- intercept + delta + overall - 0.1 + (slope - slopedelta) * newDf3$Tx
newDf3$Type <- "Generic 3"
newDf <- rbind(newDf1, newDf2, newDf3)

allCop67v2 <- ggplot(dset) + theme_bw() + 
  geom_point(aes(Tx, COP5min), size = 1, alpha = .5) +
  geom_line(data = newDf, aes(Tx, fitted, col = Type), size = 1.5) +
  ggtitle("Lab Test COP at 67F Air Temp\nAll Makes All Tests") +
  xlab("Temp at Bottom Third of Tank (F)") + ylab("5-Minute Estimated COP")
allCop67v2
ggsave(file = "graphs/allCop67_v2.png", allCop67v2, width = 7, height = 5)







# Get the COP at 50 for EVERYBODY!!!
cdx("hpwhlab")
files <- dir("../data/prepped")
files <- files[grep("50.dta", files)]


dset <- do.call('rbind', lapply(files, function(file) {
  print(file)
  tmp <- read.dta(paste("../data/prepped", file, sep = "/"))
  # Get rid of the resistance time???
  # 5 minute running res kW
  tmp$reskW5min <- stats::filter(tmp$Power_res_kW, rep(1, 5), sides = 1L)
  tmp <- tmp[tmp$reskW5min < 1.5, ]
  
  tmp$Tx <- (tmp$T_Tank_02 + tmp$T_Tank_04) / 2
  tmp <- tmp[, c("Tx", "COP5min")]
  if(sum(!is.na(tmp$COP5min))) {
    tmp <- tmp[!is.na(tmp$COP5min), ]
  } else {
    return(NULL)
  }
  tmp$file <- file
  tmp
}))

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")

dset$make <- gsub("^.+_(.+)_.+$", "\\1", dset$file)
table(dset$make)
dset$Tx <- dset$Tx * 1.8 + 32
# Makes to remove...
toRemove <- c("ATI66", "Rheem", "SandenDR40gal", "SandenDR80gal", "SandenGAU", "SandenGES", "ATI50", "Daikin")
dset <- dset[!(dset$make %in% toRemove), ]
dset <- dset[!(dset$Tx < 100 & dset$COP5min < 1.2), ]

allCop50 <- ggplot(dset) + theme_bw() + 
  geom_point(aes(Tx, COP5min, col = make), size = 1, alpha = .5) +
  geom_smooth(aes(Tx, COP5min, col = make), method = "lm", se =FALSE) +
  ggtitle("Lab Test COP at 50F Air Temp\nAll Makes All Tests") +
  xlab("Temp at Bottom Third of Tank (F)") + ylab("5-Minute Estimated COP")
allCop50
ggsave(file = "graphs/allCop50nodaikin.png", allCop50, width = 7, height = 5)

summary(lm(COP5min ~ Tx * make, data = dset))
summary(lm(COP5min ~ Tx, data = dset))

dset$make <- factor(dset$make)
mod <- lmer(COP5min ~  (1 | make) + Tx + (Tx | make), data = dset)
summary(mod)


# slope <- coef(mod <- lm(COP5min ~ Tx, dset))[2]
# intercept <- coef(mod)[1]
intercept <- fixef(mod)[1]
slope <- fixef(mod)[2]
delta <- 1.2
slopedelta <- 0.008
overall <- -.35
# slope <- -0.019
newDf1 <- data.frame("Tx" = seq(65, 130))
newDf1$fitted <- intercept - delta + overall + 0.1 + (slope + slopedelta) * newDf1$Tx
newDf1$Type <- "Generic 1"
newDf2 <- data.frame("Tx" = seq(65, 130))
newDf2$fitted <- intercept + overall + slope * newDf2$Tx
newDf2$Type <- "Generic 2"
newDf3 <- data.frame("Tx" = seq(65, 130))
newDf3$fitted <- intercept + delta + overall + (slope - slopedelta) * newDf3$Tx
newDf3$Type <- "Generic 3"
newDf <- rbind(newDf1, newDf2, newDf3)

allCop50v2 <- ggplot(dset) + theme_bw() + 
  geom_point(aes(Tx, COP5min), size = 1, alpha = .5) +
  geom_line(data = newDf, aes(Tx, fitted, col = Type), size = 1.5) +
  ggtitle("Lab Test COP at 50F Air Temp\nAll Makes All Tests") +
  xlab("Temp at Bottom Third of Tank (F)") + ylab("5-Minute Estimated COP")
allCop50v2
ggsave(file = "graphs/allCop50_v2.png", allCop50v2, width = 7, height = 5)

dset[dset$COP5min < 1.7 & dset$Tx < 80, ]


# Calculate energy factors for the DOE_24hr67 and DOE_24hr50 tests
calcEf <- function(x) {
  if(!nrow(x)) return(NA)
  if(x$test[1] == "DOE_24hr67") {
    inletT <- 58
  } else if(x$test[1] == "DOE_24hr50") {
    inletT <- 52
  } else {
    inletT <- 55
  }
  tankSize <- x$size[1]
  input_btu <- sum(x$inputPower / 60 * 3.412, na.rm = TRUE)
  delivered_btu <- sum(x$flow * 8.34 * (x$tcouples6[1] - inletT))
  delta_btu <- (x$aveTankTemp[1] - x$aveTankTemp[nrow(x)]) * tankSize * 8.34
  (delivered_btu - delta_btu) / input_btu
}
allResults <- read.csv("HpwhTestTool/allResults.csv")
allResults <- allResults[allResults$test %in% c("DOE_24hr67", "DOE_24hr50"), ]
sort(unique(allResults$model))
sizes <- data.frame("model" = sort(unique(allResults$model)),
                    "size" = c(57, 75, 42, 64, 75, 45, 45, 45, 45, 45, 45, 45, 83, 40, 56))
allResults <- merge(allResults, sizes)
efSim <- do.call('rbind', by(allResults, allResults$model, function(x) {
  data.frame("model" = x$model[1], 
             "EF" = c(calcEf(x[x$test == "DOE_24hr67", ]), calcEf(x[x$test == "DOE_24hr50", ])),
             "temp" = c(67, 50))
}))

labResults <- read.csv("HpwhTestTool/labResults.csv")
labResults <- labResults[labResults$test %in% c("DOE_24hr67", "DOE_24hr50"), ]
labResults <- merge(labResults, sizes)
efMeasured <- do.call('rbind', by(labResults, labResults$model, function(x) {
  data.frame("model" = x$model[1:2], 
             "EF" = c(calcEf(x[x$test == "DOE_24hr67", ]), calcEf(x[x$test == "DOE_24hr50", ])),
             "temp" = c(67, 50))
}))
efMeasured$type <- "Measured"
efSim$type <- "Sim"

efs <- rbind(efSim, efMeasured)
row.names(efs) <- NULL
means <- aggregate(EF ~ model, data = efs, FUN = mean)
means <- arrange(means, EF)
means$i <- 1:nrow(means)
means$EF <- NULL

efs$i <- NULL
efs <- merge(efs, means)
efs$model <- factor(efs$model, levels = means$model)

efPlot <- ggplot(efs) + theme_bw() + 
  geom_point(aes(x = EF, y = model, col = factor(temp), shape = type)) +
  ggtitle("24 Hour Test Simple Energy Factors, Measured and Simulated") +
  scale_colour_discrete(name = "Test Temp (F)") +
  scale_shape_discrete(name = "Type", labels = c("Measured", "Simulated")) +
  xlab("Simple Energy Factor") + ylab("Model") +
  scale_x_continuous(breaks = seq(1, 5, by = 0.5))
efPlot
ggsave(file = "graphs/efPlot.png", efPlot, width = 7, height = 5)






# Look at the results of the Weekly draw profiles...
weekly <- read.csv("allWeeklyResults.csv")
weekly <- merge(weekly, sizes)

ggplot(weekly[weekly$test == "Weekly_5", ]) + theme_bw() + 
  geom_line(aes(minutes, inputPower, col = model)) +
  facet_wrap(~model)

wsum <- ddply(weekly, .(model, test), calcEf)
names(wsum) <- c("model", "test", "SEF")
wsum2 <- reshape(wsum, v.names = "SEF", timevar = "test", idvar = "model", direction = "wide")
names(wsum2) <- c("model", "Weekly_1", "Weekly_2", "Weekly_3", "Weekly_4", "Weekly_5")
wsum2$average <- apply(wsum2[, -1], 1, mean)
wsum2 <- arrange(wsum2, average)
wsum2

wsum$model <- factor(wsum$model, levels = wsum2$model)
wplot <- ggplot(wsum) + theme_bw() + 
  geom_point(aes(x = SEF, y = model, col = test)) +
  geom_point(data = wsum2, aes(x = average, y = model), shape = 10) +
  ggtitle("Weekly SEEM Draw Profiles\nAll Simulated Results") +
  xlab("Simple Energy Factor") + ylab("Model") +
  scale_colour_discrete(name = "Test")
wplot
ggsave(file = "graphs/wplot.png", wplot, width = 7, height = 5)


rows <- weekly$test == "Weekly_2" & (weekly$model == "GE502014" | weekly$model == "GE502014STDMode")
ggplot(weekly[rows, ]) + theme_bw() + 
  geom_line(aes(minutes, inputPower, col = model))

ggplot(weekly[weekly$model %in% c("GE502014", "GE502014STDMode") & weekly$inputPower > 1000, ]) + theme_bw() + 
  geom_histogram(aes(inputPower, fill = model), alpha = .5)
weekly$res <- weekly$inputPower > 3900

abc <- aggregate(inputPower ~ model + test, data = weekly[weekly$res, ], FUN = sum)
abc$inputPower <- abc$inputPower / 60000
abc[grep("GE50", abc$model), ]

ggplot(weekly[rows & weekly$minutes > 5500 & weekly$minutes < 7500, ]) + theme_bw() + 
  geom_line(aes(minutes, tcouples5, col = model))


# 
# wsum <- do.call('rbind', by(weekly, interaction(weekly$model, weekly$test), function(x) {
#   y <- calcEf(x)
#   y$model <- x$model[1]
#   y$test <- x$test[1]
#   y
# }))
# 
# 
# 
# 
# weekly <- aggregate(inputPower ~ model + test, FUN = sum, data = weekly)
# weekly$inputPower <- weekly$inputPower / 60000 / 7
# 
# 
# dset <- reshape(weekly, v.names = "inputPower", timevar = "test", idvar = "model", direction = "wide")
# dset$average <- apply(dset[, -1], 1, function(x) {
#   t(x) %*% c(.2, .46, .12, .12, .1)
# })
# dset <- arrange(dset, average)
# names(dset) <- c("model", "Weekly_1", "Weekly_2", "Weekly_3", "Weekly_4", "Weekly_5", "average")
# dset
# 
# head(weekly)




