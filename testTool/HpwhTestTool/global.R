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
fieldResults <- read.csv("fieldResults.csv")

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



fieldPlot <- function(model) {
  fieldResults$X <- NULL
  fieldResults2 <- fieldResults[fieldResults$model == model, ]
  if(!nrow(fieldResults2)) {
    stop(paste("Model", model, "not simulated yet."))
  }
  flong <- reshape2::melt(fieldResults2, 
                          id.vars = c("siteid", "model"))
  flong$type <- gsub("^(.+)\\.(.+)\\.(.+)$", "\\1", flong$variable)
  flong$heatSource <- gsub("^(.+)\\.(.+)\\.(.+)$", "\\2", flong$variable)
  simRows <- grep("Sim", flong$variable)
  measRows <- grep("Measured", flong$variable)
  flong$variable <- NULL
  f2 <- tidyr::spread(flong, type, value)
  
  ggplot(f2) + theme_bw() +
    geom_point(aes(Measured, Sim, col = heatSource)) +
    geom_smooth(aes(Measured, Sim, col = heatSource), method = "lm") +
    geom_abline(slope = 1, intercept = 0, linetype = "dashed") +
    facet_wrap(~heatSource, nrow = 2, scales = "free_x")
  
#   means <- (fieldResults2$Sim.Total.kWh + fieldResults2$Measured.Total.kWh) / 2
#   
#   flong$siteid <- factor(flong$siteid, 
#                          levels = fieldResults2$siteid[sort(means, index.return = TRUE, decreasing = TRUE)$ix])
#   
#   ggplot(flong[-grep("Total", flong$variable), ]) + theme_bw() + 
#     geom_bar(aes(x = type, y = value, fill = heatSource), 
#              position = "stack", stat = "identity") +
#     facet_wrap(~siteid)
  
#   breaks1 <- c(2, 3, 5, 8, 10, 12, 15)
#   ggplot(fieldResults2) + theme_bw() + 
#     geom_point(aes(Measured.Total.kWh, Sim.Total.kWh)) + 
#     geom_smooth(aes(Measured.Total.kWh, Sim.Total.kWh), method = "lm") + 
#     geom_abline(slope = 1, intercept = 0, linetype = "dashed") +
#     scale_x_log10(breaks = breaks1) + scale_y_log10(breaks = breaks1) +
#     xlab("Measured Total kWh") + ylab("Simulated Total kWh")
#   
#   ggplot(fieldResults) + theme_bw() + 
#     geom_point(aes(Measured.Resistance.kWh, Sim.Resistance.kWh)) + 
#     geom_smooth(aes(Measured.Resistance.kWh, Sim.Resistance.kWh), method = "lm") + 
#     geom_abline(slope = 1, intercept = 0, linetype = "dashed") +
#     # scale_x_log10(breaks = breaks1) + scale_y_log10(breaks = breaks1) +
#     xlab("Measured Resistance kWh") + ylab("Simulated Resistance kWh")
#   
#   
#   ggplot(fieldResults) + theme_bw() + 
#     geom_point(aes(Measured.Compressor.kWh, Sim.Compressor.kWh)) + 
#     geom_smooth(aes(Measured.Compressor.kWh, Sim.Compressor.kWh), method = "lm") + 
#     geom_abline(slope = 1, intercept = 0, linetype = "dashed") +
#     scale_x_log10(breaks = breaks1) + scale_y_log10(breaks = breaks1) +
#     xlab("Measured Compressor kWh") + ylab("Simulated Compressor kWh")
#   
#   
}



