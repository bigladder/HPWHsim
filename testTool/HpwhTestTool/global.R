# global.R for the hpwh Test Tool. Run simulations, blah blah blah
library(ggplot2)

#
# setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")
setwd("/storage/server/nkvaltine/Projects/HPWHsim/testTool/")
# source("./runSims.R")
# rm(list = ls())
# setwd("/storage/server/nkvaltine/Projects/HPWHsim/testTool/HpwhTestTool")
#
# setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")
# source("../collectLabResults.R")
# rm(list = ls())
#
# setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")

# modelsToUse <- c("GEred", "Voltex60", "Voltex80", "Sanden80", "AOSmithHPTU66")
# testsToUse <- c("DOE_24hr50", "DOE_24hr67", "DP_SHW50", "DOE2014_24hr50", "DOE2014_24hr67")
# seemTests <- c(paste("Daily", 1:5, sep = "_"), paste("Weekly", 1:5, sep = "_"))
# seemTests <- paste("Daily", 1:5, sep = "_")
# testsToUse <- c(testsToUse, seemTests)


# allSimResults <- read.csv("allResults.csv")
# allLabResults <- read.csv("allLabResults.csv")
allSimResults <- read.csv("simResults.csv")
allLabResults <- read.csv("labResults.csv")
allChipResults <- read.csv("allChipResults.csv")
fieldResults <- read.csv("fieldResults.csv")

varGuide <- data.frame("variable" = c("flow", "inputPower", "outputPower",
                                      "tcouples1", "tcouples2", "tcouples3",
                                      "tcouples4", "tcouples5", "tcouples6",
                                      "aveTankTemp", "Ta"),
                       "units" = c("Gallons", rep("Watts", 2),
                                   rep("Fahrenheit", 8)),
                       "category" = c("Draw", "Input Power", "Output Power",
                                      rep("Thermocouples", 6), "Average Tank Temp",
                                      "Ambient Temp"))

allSimLong <- reshape2::melt(allSimResults, id.vars = c("minutes", "test", "model", "type"))
allLabLong <- reshape2::melt(allLabResults, id.vars = c("minutes", "test", "model", "type"))
# allChipLong <- reshape2::melt(allChipResults, id.vars = c("minutes", "test", "model", "type"))

# allLong <- rbind(allSimLong, allLabLong, allChipLong)
allLong <- rbind(allSimLong, allLabLong)
# rm(allSimLong, allLabLong, allChipLong)
rm(allSimLong, allLabLong)
allLong <- merge(allLong, varGuide)
# allLong <- allLong[allLong$model %in% modelsToUse, ]
# allLong <- allLong[allLong$test %in% testsToUse, ]



# Overriding model names...
modelNames <- list()
modelNames[[1]] <- list("oldname" = "GEred", "newname" = "GE2012")
modelNames[[2]] <- list("oldname" = "GE502014", "newname" = "GE2014")
modelNames[[3]] <- list("oldname" = "GE502014STDMode", "newname" = "GE2014STDMode")
modelNames[[4]] <- list("oldname" = "AOSmith60", "newname" = "AOSmithPHPT60")
modelNames[[5]] <- list("oldname" = "AOSmith80", "newname" = "AOSmithPHPT80")
modelNames[[6]] <- list("oldname" = "SandenGAU", "newname" = "Sanden80")
modelNames[[7]] <- list("oldname" = "SandenGES", "newname" = "Sanden40")
modelNames[[8]] <- list("oldname" = "Stiebel220e", "newname" = "Stiebel220E")
allLong$model <- as.character(allLong$model)
fieldResults$model <- as.character(fieldResults$model)
for(i in seq_along(modelNames)) {
  oldname <- modelNames[[i]]$oldname
  newname <- modelNames[[i]]$newname
  allLong$model[allLong$model == oldname] <- newname
  fieldResults$model[fieldResults$model == oldname] <- newname
}
allLong$model <- factor(allLong$model)

# model <- "Voltex60"
# test <- "DOE_24hr67"
onePlot <- function(model, test, vars, tmin = 0, tmax = 1) {
  dset <- allLong[allLong$model == model & allLong$test == test, ]

  totalMinutes <- max(dset$minutes)
  dset <- dset[dset$minutes >= tmin * 60 & dset$minutes <= tmax * 60, ]

  inputPowerSim <- sum(dset$value[dset$variable == "inputPower" & dset$type == "Simulated"] / 60, na.rm = TRUE) / 1000
  inputPowerMeas <- sum(dset$value[dset$variable == "inputPower" & dset$type == "Measured"] / 60, na.rm = TRUE) / 1000
  inputPowerSim <- round(inputPowerSim, 2)
  inputPowerMeas <- round(inputPowerMeas, 2)

  dset <- merge(dset, varGuide[varGuide$category %in% vars, ])
  dset$variable <- factor(dset$variable, levels = unique(as.character(dset$variable)))

#   c("Thermocouples", "Average Tank Temp",
#     "Draw", "Input Power", "Output Power")
  lineVars <- data.frame("vname" = c("aveTankTemp", "flow", "inputPower", "Ta"),
                         "var" = c("Average Tank Temp", "Draw", "Input Power", "Ambient Temp"))
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

  p <- p + ggtitle(paste("kWh Measured:", inputPowerMeas,
                         " kWh Simulated:", inputPowerSim))
  p
}
# onePlot("GEred", "DOE_24hr50", "Thermocouples")

# model <- "Voltex60"; test = "Daily_1"; vars = c("Thermocouples", "Input Power", "Output Power"); tmin = 0; tmax = 24;
oneChipPlot <- function(model, test, vars, tmin = 0, tmax = 1) {
    dset <- allLong[allLong$model == model & allLong$test == test, ]

    totalMinutes <- max(dset$minutes)
    dset <- dset[dset$minutes >= tmin * 60 & dset$minutes <= tmax * 60, ]

    inputPowerSim <- sum(dset$value[dset$variable == "inputPower" & dset$type == "Simulated"] / 60, na.rm = TRUE) / 1000
    inputPowerChip <- sum(dset$value[dset$variable == "inputPower" & dset$type == "CSE"] / 60, na.rm = TRUE) / 1000
    inputPowerSim <- round(inputPowerSim, 2)
    inputPowerChip <- round(inputPowerChip, 2)

    dset <- merge(dset, varGuide[varGuide$category %in% vars, ])
    dset$variable <- factor(dset$variable, levels = unique(as.character(dset$variable)))

    #   c("Thermocouples", "Average Tank Temp",
    #     "Draw", "Input Power", "Output Power")
    lineVars <- data.frame("vname" = c("aveTankTemp", "flow", "inputPower", "Ta"),
                           "var" = c("Average Tank Temp", "Draw", "Input Power", "Ambient Temp"))
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
      p <- p + scale_shape_discrete(name = "Type")
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

    p <- p + ggtitle(paste("kWh CSE:", inputPowerChip,
                           " kWh Simulated:", inputPowerSim))
    p
}


fieldPlot <- function(model) {
  dontUse <- c(10441, 13438, 23666, 90023, 90051) # Flip Flop Sites
  fieldResults$X <- NULL
  if(model == "AOSmith60" | model == "AOSmithPHPT60") {
    model <- "Voltex60"
  } else if(model == "AOSmith80" | model == "AOSmithPHPT80") {
    model <- "Voltex80"
  }
  fieldResults2 <- fieldResults[fieldResults$model == model, ]
  fieldResults2 <- fieldResults2[!(fieldResults2$siteid %in% dontUse), ]
  if(!nrow(fieldResults2)) {
    stop(paste("No field data for", model))
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
    facet_wrap(~heatSource, nrow = 2, scales = "free") +
    xlab("Measured kWh / Day") + ylab("Simulated kWh / Day")

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





#
# hpwhProperties <- read.csv("hpwhProperties.csv")
# if(test == "DOE_24hr50") {
#   vname <- "EF50"
# } else if(test == "DOE2014_24hr50") {
#   vname <- "EF50_DOE2014"
# } else if(test == "DOE_24hr67") {
#   vname <- "EF67"
# } else if(test == "DOE2014_24hr67") {
#   vname <- "EF67_DOE2014"
# }
