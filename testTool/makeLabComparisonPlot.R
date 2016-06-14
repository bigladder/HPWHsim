library(plyr)

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



collectSimData <- function(make) {
  print(paste("Collecting simulated data for", make))
  # Read all of the simulated results...
  files <- dir(paste("tests", make, sep = "/"), pattern = "^TestToolOutput.csv",
               recursive = TRUE, full.names = TRUE)
  if(!length(files)) return(NULL)
  weeklyFiles <- grep("Weekly", files)
  if(length(weeklyFiles)) files <- files[-weeklyFiles]
  simResults <- do.call('rbind', lapply(files, function(f) {
    tmp <- read.csv(f)
    test <- gsub("^.+/(.+)_(.+)/TestToolOutput.csv$", "\\1_\\2", f)
    tmp$test <- test
    tmp$model <- make
    if(length(grep("1hr", test))) {
      tmp <- tmp[1:60, ]
    }
    tmp
  }))
  
  # Clean up the sim results
  # Parse the energy outputs
  inputVars <- grep("input_kWh", names(simResults))
  if(length(inputVars) > 1) {
    simResults$inputTotal_W <- apply(simResults[, inputVars], 1, sum) * 60000
  } else {
    simResults$inputTotal_W <- simResults[, inputVars] * 60000
  }
  simResults <- simResults[, -inputVars]
  
  outputVars <- grep("output_kWh", names(simResults))
  if(length(outputVars) > 1) {
    simResults$outputTotal_W <- apply(simResults[, outputVars], 1, sum) * 60000
  } else {
    simResults$outputTotal_W <- simResults[, outputVars] * 60000
  }
  simResults <- simResults[, -outputVars]
  
  tempVars <- grep("Tcouple", names(simResults))
  simResults[, tempVars] <- simResults[, tempVars] * 1.8 + 32
  simResults$aveTankTemp <- apply(simResults[, tempVars], 1, mean)
  
  
  names(simResults)[names(simResults) == "draw"] <- "flow"
  names(simResults)[names(simResults) == "simTcouples1"] <- "tcouples1"
  names(simResults)[names(simResults) == "simTcouples2"] <- "tcouples2"
  names(simResults)[names(simResults) == "simTcouples3"] <- "tcouples3"
  names(simResults)[names(simResults) == "simTcouples4"] <- "tcouples4"
  names(simResults)[names(simResults) == "simTcouples5"] <- "tcouples5"
  names(simResults)[names(simResults) == "simTcouples6"] <- "tcouples6"
  simResults$inputPower <- simResults$inputTotal_W
  simResults$outputPower <- simResults$outputTotal_W
  
  simResults <- simResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                               "tcouples1", "tcouples2", "tcouples3",
                               "tcouples4", "tcouples5", "tcouples6",
                               "aveTankTemp")]
  simResults$type <- "Simulated"
  simResults
}

makes <- c("Sanden40", "Sanden80")
simData <- do.call('rbind', lapply(makes, collectSimData))
write.csv(file = "simResults.csv", simData, row.names = FALSE)



varGuide <- data.frame("variable" = c("flow", "inputPower", "outputPower",
                                      "tcouples1", "tcouples2", "tcouples3",
                                      "tcouples4", "tcouples5", "tcouples6",
                                      "aveTankTemp", "Ta"),
                       "units" = c("Gallons", rep("Watts", 2),
                                   rep("Fahrenheit", 8)),
                       "category" = c("Draw", "Input Power", "Output Power",
                                      rep("Thermocouples", 6), "Average Tank Temp",
                                      "Ambient Temp"))




allLabResults <- read.csv("allLabResults.csv")
allSimResults <- read.csv("simResults.csv")


allSimLong <- reshape2::melt(allSimResults, id.vars = c("minutes", "test", "model", "type"))
allLabLong <- reshape2::melt(allLabResults, id.vars = c("minutes", "test", "model", "type"))

#rename lab models - sandenGAU and SandenGES to sanden80 and sanden40
# allLabLong[allLabLong$model == "SandenGAU", "model"] <- as.factor("Sanden80")
allLabLong$model <- revalue(allLabLong$model, c("SandenGAU" = "Sanden80"))
allLabLong$model <- revalue(allLabLong$model, c("SandenGES" = "Sanden40"))

allLong <- {}
allLong <- rbind(allSimLong, allLabLong)
allLong <- merge(allLong, varGuide)


vars <- c("Thermocouples", "Average Tank Temp", "Input Power", "Draw", "Output Power", "Ambient Temp")

model <- "Sanden40"
model <- "Sanden80"
test <- "DOE_24hr17"
test <- "DOE_24hr35"
test <- "DOE_24hr50"
test <- "DOE_24hr67"
test <- "DOE_24hr95"

onePlot(model, test, vars, tmin = 0, tmax = 10) 



