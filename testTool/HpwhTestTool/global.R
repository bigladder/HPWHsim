# global.R for the hpwh Test Tool. Run simulations, blah blah blah
library(ggplot2)


# setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")
# setwd("/storage/server/nkvaltine/Projects/HPWHsim/testTool/")
# source("./runSims.R")
# rm(list = ls())
# setwd("/storage/server/nkvaltine/Projects/HPWHsim/testTool/HpwhTestTool")

# setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")
# source("../collectLabResults.R")
# rm(list = ls())
# 
# setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")


allSimResults <- read.csv("allResults.csv")
allLabResults <- read.csv("allLabResults.csv")

varGuide <- data.frame("variable" = c("flow", "inputPower", "outputPower",
                                      "tcouples1", "tcouples2", "tcouples3",
                                      "tcouples4", "tcouples5", "tcouples6",
                                      "aveTankTemp"),
                       "units" = c("Gallons", rep("Watts", 2), 
                                   rep("Fahrenheit", 7)),
                       "category" = c("Draw", "Input Power", "Output Power",
                                      rep("Thermocouples", 6), "Average Tank Temp"))

allSimLong <- reshape2::melt(allSimResults, id.vars = c("minutes", "test", "model", "type"))
allLabLong <- reshape2::melt(allLabResults, id.vars = c("minutes", "test", "model", "type"))

allLong <- rbind(allSimLong, allLabLong)
allLong <- merge(allLong, varGuide)


# model <- "Voltex60"
# test <- "DOE_24hr67"
onePlot <- function(model, test, vars, tmin = 0, tmax = 1) {
  dset <- allLong[allLong$model == model & allLong$test == test, ]

  totalMinutes <- max(dset$minutes)
  dset <- dset[dset$minutes >= tmin * 60 & dset$minutes <= tmax * 60, ]
  
  dset <- merge(dset, varGuide[varGuide$category %in% vars, ])
  dset$variable <- factor(dset$variable, levels = unique(as.character(dset$variable)))

#   c("Thermocouples", "Average Tank Temp",
#     "Draw", "Input Power", "Output Power")
  lineVars <- data.frame("vname" = c("aveTankTemp", "flow", "inputPower"),
                         "var" = c("Average Tank Temp", "Draw", "Input Power"))
  lineVars <- lineVars[lineVars$var %in% vars, ]

  dset$colourVar <- paste(dset$type, dset$category)
    
  colourScale <- 0
  p <- ggplot(dset) + theme_bw()
  if("Thermocouples" %in% vars) {
    p <- p + geom_line(data = dset[grep("tcouples", dset$variable), ],
                       aes(minutes, value, group = interaction(variable, type), linetype = type),
                       colour = "grey")
  } 
  if("Output Power" %in% vars) {
    p <- p + geom_point(data = dset[grep("output", dset$variable), ], 
               aes(minutes, value, col = colourVar, shape = type))
    colourScale <- 1
  } 
  if(nrow(lineVars)) {
    p <- p + geom_line(data = dset[dset$variable %in% lineVars$vname, ],
                           aes(minutes, value, col = colourVar, linetype = type))
    colourScale <- 1
  }
  if(colourScale) {
    p <- p + scale_colour_discrete(name = "Variable")
  }

  p <- p + facet_wrap(~units, scales = "free_y", ncol = 1) +
    xlab("Minutes Into Test") + ylab("Value") +
    scale_linetype_discrete(name = "Type")
  p
}

