# global.R for the hpwh Test Tool. Run simulations, blah blah blah
library(ggplot2)

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")
source("../runSims.R")
rm(list = ls())

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")
source("../collectLabResults.R")
rm(list = ls())

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")


allSimResults <- read.csv("allResults.csv")
allLabResults <- read.csv("allLabResults.csv")

varGuide <- data.frame("variable" = c("flow", "inputPower", "outputPower",
                                      "tcouples1", "tcouples2", "tcouples3",
                                      "tcouples4", "tcouples5", "tcouples6",
                                      "aveTankTemp"),
                       "units" = c("Gallons", rep("Watts", 2), 
                                   rep("Fahrenheit", 7)))

allSimLong <- reshape2::melt(allSimResults, id.vars = c("minutes", "test", "model", "type"))
allLabLong <- reshape2::melt(allLabResults, id.vars = c("minutes", "test", "model", "type"))

allLong <- rbind(allSimLong, allLabLong)
allLong <- merge(allLong, varGuide)


model <- "Voltex60"
test <- "DOE_24hr67"
onePlot <- function(model, test, tmax = 1) {
  dset <- allLong[allLong$model == model & allLong$test == test, ]

  totalMinutes <- max(dset$minutes)
  dset <- dset[dset$minutes <= totalMinutes * tmax, ]

  ggplot(dset) + theme_bw() + 
    geom_line(data = dset[grep("tcouples", dset$variable), ],
              aes(minutes, value, group = interaction(variable, type), linetype = type),
              colour = "grey") +
    geom_line(data = dset[-grep("tcouples", dset$variable), ],
              aes(minutes, value, col = interaction(variable, type), linetype = type)) +
    geom_point(data = dset[grep("output", dset$variable), ], 
               aes(minutes, value, col = interaction(variable, type), shape = type)) +
    facet_wrap(~units, scales = "free_y", nrow = 3)
  
}

onePlot(model, test, 400)


head(allSimResults[allSimResults$model == model & allSimResults$test == test, ])
head(allLabResults[allLabResults$model == model & allLabResults$test == test, ])


