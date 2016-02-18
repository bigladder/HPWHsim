library(plyr)
library(ggplot2)

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")

allSimResults <- read.csv("HpwhTestTool/allResults.csv")
allChipResults <- read.csv("HpwhTestTool/allChipResults.csv")

allChipResults$testModel <- paste(allChipResults$test, allChipResults$model, sep = ":")
allSimResults$testModel <- paste(allSimResults$test, allSimResults$model, sep = ":")

tcoupleVars <- grep("tcouples", names(allSimResults))
allSimResults[, tcoupleVars] <- round((allSimResults[, tcoupleVars] - 32) / 1.8, 2)
tcoupleVars <- grep("tcouples", names(allChipResults))
allChipResults[, tcoupleVars] <- round((allChipResults[, tcoupleVars] - 32) / 1.8, 2)
powerVars <- grep("Power", names(allSimResults))
allSimResults[, powerVars] <- round(allSimResults[, powerVars] / 60, 2)
powerVars <- grep("Power", names(allChipResults))
allChipResults[, powerVars] <- round(allChipResults[, powerVars] / 60, 2)


testModels <- unique(allChipResults$testModel)

allSimResults <- allSimResults[allSimResults$testModel %in% testModels, ]

vars <- c("inputPower", "outputPower", "tcouples1", "tcouples2", "tcouples3",
          "tcouples4", "tcouples5", "tcouples6", "aveTankTemp")
lapply(testModels, function(tm) {
  simTmp <- allSimResults[allSimResults$testModel == tm, ]
  chipTmp <- allChipResults[allChipResults$testModel == tm, ]
  chipTmp <- chipTmp[1:nrow(simTmp), ]
  # In case Chip provided extra...

  # Make sure the sorting is by minute
  simTmp$minutes <- simTmp$minutes + 1
  simTmp <- arrange(simTmp, minutes)
  chipTmp <- arrange(chipTmp, minutes)
  
  sapply(vars, function(var) {
    sum((chipTmp[, var] - simTmp[, var]) ^ 2)
  })
  
})

tm <- testModels
simTmp <- allSimResults[allSimResults$testModel == tm, ]
simTmp$minutes <- simTmp$minutes + 1
chipTmp <- allChipResults[allChipResults$testModel == tm, ]
chipTmp <- chipTmp[1:nrow(simTmp), ]

df <- rbind(simTmp[, c("minutes", "inputPower", "type")],
            chipTmp[, c("minutes", "inputPower", "type")])
df2 <- reshape(df, v.names = "inputPower", idvar = "minutes",
               timevar = "type", direction = "wide")
df2$diff <- df2$inputPower.CSE - df2$inputPower.Simulated

input1Plot <- ggplot(df2) + theme_bw() + 
  geom_point(aes(minutes, diff)) +
  ggtitle("Daily_1 Voltex 60 Input Power Difference\nEcotope Simulated vs CSE Simulated") +
  xlab("Minutes into Test") +
  ylab("Difference between Ecotope Watts and CSE Watts")
input1Plot
ggsave(file = "graphs/input1Plot.png", input1Plot, width = 7, height = 5)

input2Plot <- ggplot(df2[df2$minutes > 800 & df2$minutes < 950, ]) + theme_bw() + 
  geom_line(aes(minutes, diff)) +
  ggtitle("Daily_1 Voltex 60 Input Power Difference\nEcotope Simulated vs CSE Simulated") +
  xlab("Minutes into Test") +
  ylab("Difference between Ecotope Watts and CSE Watts")
input2Plot
ggsave(file = "graphs/input2Plot.png", input2Plot, width = 7, height = 5)












 #Old
df$diff <- df$chip - df$sim
plot(df$i, df$diff)

sum(chipTmp$flow != simTmp$flow)
sum(chipTmp$tcouples1 != simTmp$tcouples1)

df <- data.frame("chip" = chipTmp$tcouples1, "sim" = simTmp$tcouples1)
df$diff <- df$chip - df$sim
df$i <- 1:nrow(df)

art <- ggplot(df) + theme_bw() +
  geom_point(aes(i, diff)) +
  xlab("") + ylab("") + 
  ggtitle("Reconciling Simulations - Modern Art")
art
# ggsave(file = "graphs/art.png", art, width = 7, height = 5)
