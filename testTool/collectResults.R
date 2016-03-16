

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")


collectLabData <- function(make) {
  print(paste("Collecting lab data for", make))
  # Read all of the actual results...
  files <- dir(paste("models", make, sep = "/"), pattern = "Full.csv", full.names = TRUE)
  if(!length(files)) return(NULL)
  labResults <- do.call('rbind', lapply(files, function(f) {
    tmp <- read.csv(f)
    test <- gsub("^.+/(.+)_(.+)_Full.csv$", "\\1_\\2", f)
    tmp$test <- test
    tmp$model <- make
    if(length(grep("1hr", test))) {
      tmp <- tmp[1:60, ]
    }
    tmp
  }))
  
  # Clean up the naming convention & variables
  names(labResults)[names(labResults) == "flow_out_gal"] <- "flow"
  names(labResults)[names(labResults) == "T_Tank_02"] <- "tcouples1"
  names(labResults)[names(labResults) == "T_Tank_04"] <- "tcouples2"
  names(labResults)[names(labResults) == "T_Tank_06"] <- "tcouples3"
  names(labResults)[names(labResults) == "T_Tank_08"] <- "tcouples4"
  names(labResults)[names(labResults) == "T_Tank_10"] <- "tcouples5"
  names(labResults)[names(labResults) == "T_Tank_12"] <- "tcouples6"
  labResults$inputPower <- labResults$Power_W
  labResults$outputPower <- labResults$dEadd5min / 5 / 60 * 1000
  
  tempVars <- grep("tcouples", names(labResults))
  # labResults[, tempVars] <- labResults[, tempVars] * 1.8 + 32
  labResults$aveTankTemp <- apply(labResults[, tempVars], 1, mean)
  labResults$Ta <- labResults$T_Plenum_In * 1.8 + 32
  
  labResults <- labResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                               "tcouples1", "tcouples2", "tcouples3",
                               "tcouples4", "tcouples5", "tcouples6",
                               "aveTankTemp", "Ta", "COP5min")]
  labResults$type <- "Measured"
  labResults
}

collectSimData <- function(make) {
  print(paste("Collecting simulated data for", make))
  # Read all of the simulated results...
  files <- dir(paste("models", make, sep = "/"), pattern = "TestToolOutput.csv",
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

makes <- c("AOSmith60", "AOSmith80",
           "AOSmithHPTU50", "AOSmithHPTU66", "AOSmithHPTU80",
           "GEred", "GE502014", "GE502014STDMode", "RheemHB50", 
           "SandenGAU", "SandenGES", "Stiebel220e",
           "Generic1", "Generic2", "Generic3")

labData <- do.call('rbind', lapply(makes, collectLabData))
simData <- do.call('rbind', lapply(makes, collectSimData))


# Remove certain tests...
# Exceptions...
exceptions <- list()
exceptions[[1]] <- list("model" = "GE502014STDMode",
                        "dontuse" = c("COP_24hr50", "COP_24hr67", "COP_new24hr50", "COP_new24hr50b", "DOE_24hr50noCurtain"))
exceptions[[2]] <- list("model" = "GE502014",
                        "dontuse" = c("CMP_T", "COP_24hr50", "COP_new24hr67", "COP_old24hr50", "COP_old24hr67"))
exceptions[[3]] <- list("model" = "AOSmith60",
                        "dontuse" = c("COP_67"))
exceptions[[4]] <- list("model" = "AOSmith80",
                        "dontuse" = c("COP_30", "COP_40Auto", "DP_4T40", "DP_4T67", "DP_4T67Duct"))
exceptions[[5]] <- list("model" = "AOSmithHPTU50",
                        "dontuse" = c("COMP_T"))
exceptions[[6]] <- list("model" = "SandenGAU",
                        "dontuse" = c("COP_50", "COP_67"))
exceptions[[7]] <- list("model" = "SandenGES",
                        "dontuse" = c("COP_30", "COP_50", "COP_67"))
exceptions[[8]] <- list("model" = "RheemHB50",
                        "dontuse" = c("CMP_T", "CMP_T37", "CMP_T37r2", "CMP_T38", "CMP_T39", "CMP_T39r2", "CMP_T40", "CMP_T42"))
exceptions[[9]] <- list("model" = "Stiebel220e",
                        "dontuse" = c("CMP_T", "DOE2014_1hrhres", "DOE2014_24hr50hres", "DOE2014_24hr67hres", "DOE_1hr", "DOE_1hrhres", "DOE_24hr50hres", "DOE_24hr67hres", "DP_SHW50hres"))
exceptions[[10]] <- list("model" = "Generic3",
                         "dontuse" = c("DOE_1hr", "DOE2014_1hr"))
exceptions[[11]] <- list("model" = "Generic2",
                         "dontuse" = c("DOE_1hr", "DOE2014_1hr"))
exceptions[[12]] <- list("model" = "Generic1",
                         "dontuse" = c("DOE_1hr", "DOE2014_1hr"))
for(i in seq_along(exceptions)) {
  model <- exceptions[[i]]$model
  for(test in exceptions[[i]]$dontuse) {
    noRowsLab <- which(labData$model == model & labData$test == test)
    if(length(noRowsLab)) {
      labData <- labData[-noRowsLab, ]
    }
    noRowsSim <- which(simData$model == model & simData$test == test)
    if(length(noRowsSim)) {
      simData <- simData[-noRowsSim, ]
    }
  }
}



write.csv(file = "HpwhTestTool/labResults.csv", labData, row.names = FALSE)
write.csv(file = "HpwhTestTool/simResults.csv", simData, row.names = FALSE)




