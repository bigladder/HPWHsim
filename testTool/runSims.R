# Make all of the output...
setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")
library(EcotopePackage)
library(foreign)


# tests <- c("DOE_24hr50", "DOE_24hr67", "DP_SHW50", "DOE2014_24hr67", "DOE2014_24hr50")
tests <- c("DrawProfileTest_4p1_24hr67")
# seemTests <- c(paste("Daily", 1:5, sep = "_"), paste("Weekly", 1:5, sep = "_"))
# seemTests <- paste("Daily", 1:5, sep = "_")
# tests <- c(tests, seemTests)
# models <- c("Voltex60", "Voltex80", "ATI66", "GEred", "Sanden80", "GE2014", "GE")
models <- c("AOSmith60", "AOSmith80",
            "AOSmithHPTU50", "AOSmithHPTU66", "AOSmithHPTU80",
            "GEred", "GE502014", "GE502014STDMode", "RheemHB50", 
            "SandenGAU", "SandenGES", "Stiebel220e",
            "Generic1", "Generic2", "Generic3")


# Simulate every combination of test and model
allResults <- do.call('rbind', lapply(models, function(model) {
  # Loop over tests...
  tests <- dir(paste("models", model, sep = "/"), pattern = "^[^_]+_[^_]+$")
  weeklyTests <- grep("Weekly", tests)
  if(length(weeklyTests)) tests <- tests[-weeklyTests]
  do.call('rbind', lapply(tests, function(test) {
    print(paste("Simulating model", model, "test", test))
    # Run the Simulation
    system(paste("./testTool.x", test, model))
    
    # Read the results
    dset <- try(read.csv(paste("models/", model, "/", test, "/", "TestToolOutput.csv", sep = "")))
    if(inherits(dset, "try-error")) {
      print(paste("No Output for", model, test)) 
      return(NULL)
    }
    dset$test <- test
    dset$model <- model
    
    # Parse the energy outputs
    inputVars <- grep("input_kWh", names(dset))
    if(length(inputVars) > 1) {
      dset$inputTotal_W <- apply(dset[, inputVars], 1, sum) * 60000
    } else {
      dset$inputTotal_W <- dset[, inputVars] * 60000
    }
    dset <- dset[, -inputVars]
    
    outputVars <- grep("output_kWh", names(dset))
    if(length(outputVars) > 1) {
      dset$outputTotal_W <- apply(dset[, outputVars], 1, sum) * 60000
    } else {
      dset$outputTotal_W <- dset[, outputVars] * 60000
    }
    dset <- dset[, -outputVars]
    # print(head(dset))
    dset
  }))
}))

# Convert all temperature variables from C to F
tempVars <- grep("Tcouple", names(allResults))
allResults[, tempVars] <- allResults[, tempVars] * 1.8 + 32
allResults$aveTankTemp <- apply(allResults[, tempVars], 1, mean)

# inputVars <- grep("input_kWh", names(allResults))
# if(length(inputVars) > 1) {
#   allResults$inputTotal_W <- apply(allResults[, inputVars], 1, sum) * 60000
# } else {
#   allResults$inputTotal_W <- allResults[, inputVars]
# }
# outputVars <- grep("output_kWh", names(allResults))
# if(length(outputVars) > 1) {
#   allResults$outputTotal_W <- apply(allResults[, outputVars], 1, sum) * 60000  
# } else {
#   allResults$outputTotal_W <- allResults[, outputVars]
# }


allResults[] <- lapply(names(allResults), function(v) {
  if(length(grep("input_kWh", v)) | length(grep("output_kWh", v))) {
    NULL
  } else {
    allResults[, v]
  }
})


names(allResults)[names(allResults) == "draw"] <- "flow"
names(allResults)[names(allResults) == "simTcouples1"] <- "tcouples1"
names(allResults)[names(allResults) == "simTcouples2"] <- "tcouples2"
names(allResults)[names(allResults) == "simTcouples3"] <- "tcouples3"
names(allResults)[names(allResults) == "simTcouples4"] <- "tcouples4"
names(allResults)[names(allResults) == "simTcouples5"] <- "tcouples5"
names(allResults)[names(allResults) == "simTcouples6"] <- "tcouples6"
allResults$inputPower <- allResults$inputTotal_W
allResults$outputPower <- allResults$outputTotal_W

allResults <- allResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                             "tcouples1", "tcouples2", "tcouples3",
                             "tcouples4", "tcouples5", "tcouples6",
                             "aveTankTemp", "Ta")]
allResults$type <- "Simulated"

write.csv(file = "HpwhTestTool/allResults.csv", allResults, row.names = FALSE)




# Run the weekly stuff...
# Assuming that the Weekly draw profiles have been copied... see generics.R

tests <- c("Weekly_1", "Weekly_2", "Weekly_3", "Weekly_4", "Weekly_5")
models <- makes <- c("AOSmith60", "AOSmith80",
                     "AOSmithHPTU50", "AOSmithHPTU66", "AOSmithHPTU80",
                     "GEred", "GE502014", "GE502014STDMode", "RheemHB50", 
                     "SandenGAU", "SandenGES", "Stiebel220e",
                     "Generic1", "Generic2", "Generic3")

# Simulate every combination of test and model
allResults <- do.call('rbind', lapply(models, function(model) {
  # Loop over tests...
  do.call('rbind', lapply(tests, function(test) {
    print(paste("Simulating model", model, "test", test))
    # Run the Simulation
    system(paste("./testTool.x", test, model))
    
    # Read the results
    dset <- try(read.csv(paste("models/", model, "/", test, "/", "TestToolOutput.csv", sep = "")))
    if(inherits(dset, "try-error")) {
      print(paste("No Output for", model, test)) 
      return(NULL)
    }
    dset$test <- test
    dset$model <- model
    
    # Parse the energy outputs
    inputVars <- grep("input_kWh", names(dset))
    if(length(inputVars) > 1) {
      dset$inputTotal_W <- apply(dset[, inputVars], 1, sum) * 60000
    } else {
      dset$inputTotal_W <- dset[, inputVars] * 60000
    }
    dset <- dset[, -inputVars]
    
    outputVars <- grep("output_kWh", names(dset))
    if(length(outputVars) > 1) {
      dset$outputTotal_W <- apply(dset[, outputVars], 1, sum) * 60000
    } else {
      dset$outputTotal_W <- dset[, outputVars] * 60000
    }
    dset <- dset[, -outputVars]
    # print(head(dset))
    dset
  }))
}))

# Convert all temperature variables from C to F
tempVars <- grep("Tcouple", names(allResults))
allResults[, tempVars] <- allResults[, tempVars] * 1.8 + 32
allResults$aveTankTemp <- apply(allResults[, tempVars], 1, mean)


allResults[] <- lapply(names(allResults), function(v) {
  if(length(grep("input_kWh", v)) | length(grep("output_kWh", v))) {
    NULL
  } else {
    allResults[, v]
  }
})


names(allResults)[names(allResults) == "draw"] <- "flow"
names(allResults)[names(allResults) == "simTcouples1"] <- "tcouples1"
names(allResults)[names(allResults) == "simTcouples2"] <- "tcouples2"
names(allResults)[names(allResults) == "simTcouples3"] <- "tcouples3"
names(allResults)[names(allResults) == "simTcouples4"] <- "tcouples4"
names(allResults)[names(allResults) == "simTcouples5"] <- "tcouples5"
names(allResults)[names(allResults) == "simTcouples6"] <- "tcouples6"
allResults$inputPower <- allResults$inputTotal_W
allResults$outputPower <- allResults$outputTotal_W

allResults <- allResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                             "tcouples1", "tcouples2", "tcouples3",
                             "tcouples4", "tcouples5", "tcouples6",
                             "aveTankTemp", "Ta")]
allResults$type <- "Simulated"

write.csv(file = "allWeeklyResults.csv", allResults, row.names = FALSE)










# What about the field data???
runOneField <- function(siteid = "11531", model = "Voltex60") {
  # Run the simulation
  system(paste("./testTool.x ", "site", siteid, " ", model, sep = ""))
  
  # Read the Output
  simOutput <- read.csv(paste0("tests/site", siteid, "/", model, "TestToolOutput.csv"))
  fieldOutput <- read.csv(paste0("tests/site", siteid, "/fieldResults.csv"))
  names(fieldOutput)[names(fieldOutput) == "minute"] <- "minutes"
  dset <- merge(simOutput, fieldOutput)
  
  if(model == "Voltex60" | model == "Voltex80" | model == "GEred") {
    dset$simCompWatts <- dset$input_kWh2 * 60000
    dset$simResWatts <- (dset$input_kWh1 + dset$input_kWh3) * 60000    
  }
  dset$simWatts <- dset$simCompWatts + dset$simResWatts
  dset$measCompWatts <- dset$compW
  dset$measResWatts <- dset$resW
  dset$measWatts <- dset$measCompWatts + dset$measResWatts
  
  dset <- dset[, c("time", "simCompWatts", "simResWatts", "simWatts",
                   "measCompWatts", "measResWatts", "measWatts")]
  sumResults <- apply(dset[, c("simCompWatts", "simResWatts", "simWatts",
                 "measCompWatts", "measResWatts", "measWatts")], 2, mean, na.rm = TRUE) *
    24 / 1000
  sumResults <- data.frame(t(sumResults))
  names(sumResults) <- c("Sim.Compressor.kWh", "Sim.Resistance.kWh", "Sim.Total.kWh",
                         "Measured.Compressor.kWh", "Measured.Resistance.kWh", "Measured.Total.kWh")
  sumResults$siteid <- siteid
  sumResults$model <- model
  sumResults

}


cdx("hpwh")
dirTmp <- "/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/"

sites <- read.dta("../data/site_info.dta")
dontUse <- c(23860, 10441, 13438, 23666, 90023, 90051, 90134, 20814, 90069)
# Reasons to exclude: 1 bad data
# 2 through 7, flip flop sites w/ forced resistance heat
# 8 anomalous high user
# 9 probable airflow problem
setwd(dirTmp)
sites <- sites[!(sites$siteid %in% dontUse), ]

fieldResults <- lapply(1:nrow(sites), function(i) {
  siteid <- sites$siteid[i]
  if(siteid %in% dontUse) {
    return(NULL)
  }
  #Look up the make
  make <- sites$make[i]
  if(length(grep("Voltex 60", make))) {
    model <- "Voltex60"
  } else if(length(grep("Voltex 80", make))) {
    model <- "Voltex80"
  } else if(length(grep("GE", make))) {
    model <- "GEred"
  } else {
    print(paste("No simulated model for", make))
    return(NULL)
  }
  tmp <- try(runOneField(siteid, model))
  if(inherits(tmp, "try-error")) {
    NULL
  } else {
    tmp
  }
})
fieldResults <- do.call('rbind', fieldResults)
write.csv(file = "HpwhTestTool/fieldResults.csv", fieldResults)




#   
#   ggplot(dset) + theme_bw() + 
#     geom_point(aes(measCompWatts, simCompWatts), alpha = .1)
#   
#   dset$date <- as.Date(dset$time)
#   dset$hour <- lubridate::hour(dset$time) + lubridate::minute(dset$time) / 60
#   dset$wday <- lubridate::wday(dset$time, label = TRUE, abbr = FALSE)
#   dset$weekend <- dset$wday %in% c("Saturday", "Sunday")
#   dlong <- reshape2::melt(dset, id.vars = c("time", "date", "hour", "wday", "weekend"))
#   
#   hourlyDset <- aggregate(cbind(simCompWatts, simResWatts, simWatts, 
#                                 measCompWatts, measResWatts, measWatts) ~ hour + weekend,
#                           data = dset, FUN = mean)
#   hlong <- reshape2::melt(hourlyDset, id.vars = c("hour", "weekend"))
#   hlong$type <- "Measured"
#   hlong$type[grep("sim", hlong$variable)] <- "Simulated"
#   
#   ggplot(hlong[!(hlong$variable %in% c("simWatts", "measWatts")), ]) + theme_bw() +
#     geom_line(aes(hour, value, col = variable, linetype = type)) + 
#     facet_wrap(~weekend, nrow = 2)
#   
#   ggplot(hlong[(hlong$variable %in% c("simWatts", "measWatts")), ]) + theme_bw() +
#     geom_line(aes(hour, value, col = variable, linetype = type)) +
#     facet_wrap(~weekend, nrow = 2) 
#   
#   daily <- aggregate(cbind(simCompWatts, simResWatts, simWatts, 
#                            measCompWatts, measResWatts, measWatts) ~ date,
#                      data = dset, FUN = mean)
#   dlong2 <- reshape2::melt(daily, id.vars = c("date"))
#   dlong2$value <- dlong2$value * 24 / 1000
#   
#   ggplot(dlong2[dlong2$variable %in% c("simWatts", "measWatts"), ]) + theme_bw() + 
#     geom_line(aes(date, value, col = variable))
#   
#   daily[, -1] <- daily[, -1] * 24 / 1000
#   ggplot(daily) + theme_bw() + 
#     geom_point(aes(measWatts, simWatts, col = measResWatts)) +
#     geom_smooth(aes(measWatts, simWatts), method = "lm") +
#     geom_abline(slope = 1, intercept = 0, linetype = "dashed") +
#     xlab("Measured Daily kWh") +
#     ylab("Simulated Daily kWh")
#   
#   ggplot(daily) + theme_bw() + 
#     geom_point(aes(measCompWatts, simCompWatts, col = measResWatts)) +
#     geom_smooth(aes(measCompWatts, simCompWatts), method = "lm") +
#     geom_abline(slope = 1, intercept = 0, linetype = "dashed") +
#     xlab("Measured Daily Compressor kWh") +
#     ylab("Simulated Daily Compressor kWh")
#   
#   
#   ggplot(daily) + theme_bw() + 
#     geom_point(aes(measResWatts, simResWatts, col = measResWatts)) +
#     geom_smooth(aes(measResWatts, simResWatts), method = "lm") +
#     geom_abline(slope = 1, intercept = 0, linetype = "dashed") +
#     xlab("Measured Daily Compressor kWh") +
#     ylab("Simulated Daily Compressor kWh")  