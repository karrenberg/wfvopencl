library(stats)
## Copyrigh UVSQ 2009, under GPL.
## Authors: Sebastien Briais, Sid-Ahmed-Ali Touati
## input csv file
config.ifile <- NA
## output csv file
config.ofile <- NA
## output result file
config.rfile <- ""
## warning file
config.wfile <- NA

## default confidence level
config.level <- NA

## desired precision of confidence interval
config.precision <- NA

## weight
config.weight <- "custom"

## verbose mode
config.verbose <- FALSE

err.msg <- function(msg) {
  cat(msg)
}

## read config file and perform basic checks
## file = filename
## return a data frame 
speedup.read.config <- function(file) {
  bench.cfg <- read.csv(file)

  ## check the header of configuration file
  x <- colnames(bench.cfg)
  y <- c("Name","Sample1","Sample2","ConfLevel","Coef")

  ok <- TRUE;
  
  for(i in 1:length(y)) {
    n <- y[i]
    if(length(x[x %in% n]) != 1) {
      ok <- FALSE;
      err.msg("Invalid configuration file ('",n,"' column missing?)\n")
    }
  }

  if(!ok) {
    stop("Given configuration file is invalid... aborting.\n");
  }

  if(length(x) != length(y)) {
    err.msg("Wrong number of columns in configuration file... ignoring extra-columns.\n")
    bench.cfg <- subset(bench.cfg,select=y)
  }

  if(config.weight == "custom") {
    W <- bench.cfg[["Coef"]]
    if((typeof(W) != "double") || (length(W[W < 0]) > 0) || (sum(W) == 0)) {
      err.msg("Illegal user-defined weights, using equal weights instead.\n")
      config.weight <<- "equal" 
    } 
  }

  bench.cfg
}

warn.bench.name <- NA
warn.bench.nb <- 0
warn.total.nb <- 0

result.msg <- function(msg, append=TRUE) {
  cat(file=config.rfile, append=append, paste(msg,"\n",sep=""))
}

warn.open <- function(msg) {
  cat(file=config.wfile, paste(msg,"\n"))
  warn.total.nb <<- 0
}

warn.close <- function() {
  cat(file=config.wfile, append=TRUE, paste(warn.total.nb," warning(s).\n",sep=""))
}

warn.begin <- function(bname) {
  warn.bench.name <<- bname
  warn.bench.nb <<- 0
}

warn.end <- function() {
  warn.total.nb <<- warn.total.nb + warn.bench.nb
}

warn.msg <- function(str,silent=FALSE) {
  if(!silent) {	 
    if(warn.bench.nb == 0) {
      cat(file=config.wfile, append=TRUE, paste(warn.bench.name,":\n"),sep="")
    }
    warn.bench.nb <<- warn.bench.nb+1
    cat(file=config.wfile, append=TRUE, paste("\t",str,"\n",sep=""))
  }
}

speedup.load_bench <- function(fname) {
  data <- NULL
  if(file.access(fname, mode = 4) != 0) {
    warn.msg(paste("File '",fname,"' is not readable.",sep=""))
  } else {
    data <- read.csv(header=FALSE, fname)
    data <- data[[1]]
    data <- data[!is.na(data)]
  }
  data
}

speedup.get_observation <- function(data) {
  result <- NULL
  if(is.null(data)) {
    result <- cbind(NA,NA,NA,NA)
  } else {
    result <- cbind(min(data),max(data),mean(data),median(data))
  }
  result
}

speedup.safe_ratio <- function(x, y) {
  if(y != 0) {
    return(x/y)
  } else {
    return(NA)
  }
}

speedup.search_alpha <- function(f) {
  alphamin <- 0.01
  alphamax <- 0.5

  rmin <- f(alphamin)
  if(!is.na(rmin)) {
    if(rmin) {
      return(list(result=TRUE,alpha=alphamin))
    }
  }

  rmax <- f(alphamax) 
  if(is.na(rmax)) {
    return(list(result=NA,alpha=NA))
  } else {
    if(!rmax) {
      return(list(result=NA,alpha=0.01))
    }
  }

  ## loop invariant
  ## alphamin -> FALSE ou NA
  ## alphamax -> TRUE

  while(alphamax - alphamin > 0.01) {
    alpha <- (alphamin+alphamax)/2
    r <- f(alpha)
    if(is.na(r)) {
      r <- FALSE
    }
    if(r) {
      alphamax <- alpha
    } else {
      alphamin <- alpha
    }
  }  

  return(list(result=TRUE,alpha=ceiling(100*alphamax)/100))
}

speedup.errmsg <- ""
speedup.error_handler <- function(e) {
  speedup.errmsg <<- geterrmessage()
  NULL
}

# return TRUE when it fails
# emit warnings if needed
speedup.check_normality <- function(data, alpha, msg, silent=FALSE) {
  failed <- FALSE			
  n <- length(data)
  sw <- tryCatch(shapiro.test(data), error=speedup.error_handler)
  if(is.null(sw)) {
    warn.msg(paste("An error occured while doing normality test of ",msg,": ",speedup.errmsg,".",sep=""),silent=silent)
    failed <- TRUE
  } else {
    if(is.na(sw$p.value)) {
      warn.msg(paste(msg,": normality test failed to compute p-value.",sep=""),silent=silent)
      failed <- TRUE
    } else {
      if(sw$p.value <= alpha) {
        if(n <= 30) {
          warn.msg(paste(msg, " too small for applying the Student's t-test (speedup of the mean). Please do more than 30 observations of the executions times.",sep=""),silent=silent)
	  failed <- TRUE
        } else {
          warn.msg(paste(msg," data are not normally distributed. The indicated confidence level for speedup of the average execution time may not be accurate.",sep=""),silent=silent)
        }
      }
    }
  }
  return(failed)
}

speedup.perform_mean_test <- function(data1, data2, alpha, silent=FALSE) {
  failed <- FALSE				 
  result <- NA

  failed1 <- speedup.check_normality(data1, alpha, "Sample1", silent=silent)
  failed2 <- speedup.check_normality(data2, alpha, "Sample2", silent=silent)

  failed <- failed1 || failed2

  if(!failed) {
    ft <- tryCatch(var.test(data1,data2),error=speedup.error_handler)
    if(is.null(ft)) {
      warn.msg(paste("An error occured while doing variance statistical test:",speedup.errmsg,".",sep=""),silent=silent)
    } else {
      if(is.na(ft$p.value)) {
	warn.msg("Variance test was unable to compute a p-value.",silent=silent)
      } else {
        if(ft$p.value > alpha) {
          tt <- tryCatch(t.test(data1, data2, alternative="greater", var.equal=TRUE),error=speedup.error_handler)
        } else {
          tt <- tryCatch(t.test(data1, data2, alternative="greater"),error=speedup.error_handler)
        }
	if(is.null(tt)) {
	  warn.msg(paste("An error occured while doing Student's t-test:",speedup.errmsg,"."),silent=silent)
	} else {
	  if(is.na(tt$p.value)) {
   	    warn.msg("Student's t-test was unable to compute a p-value.",silent=silent)
          } else {
            if(tt$p.value <= alpha) {
              result <- TRUE
            } else {
              result <- FALSE
            }
          }
        }
      }
    }
  }
  return(result)
}

speedup.mean_test <- function(bcfg, data1, data2) {
  if(is.na(config.level)) {
    level <- bcfg[1,"ConfLevel"]
    if((typeof(level) != "double") || (is.na(level))) {
      alpha <- NA
    } else {
      alpha <- 1 - level
    }
  } else {
    alpha <- 1 - config.level
  }
  if(is.na(alpha)) {
    x <- speedup.search_alpha(function(alpha) { speedup.perform_mean_test(data1, data2, alpha, silent=TRUE) })
    result <- x$result
    alpha <- x$alpha
    if(is.na(result)) {
      speedup.perform_mean_test(data1, data2, 0.5)
      warn.msg("Unable to find a confidence level greater than 50% to guarantee the statistical significance of mean speedup.")
    } else {
      result <- speedup.perform_mean_test(data1, data2, alpha)
    }
  } else {
    result <- speedup.perform_mean_test(data1, data2, alpha)
  }
  if(is.na(result)) {
    result <- FALSE
  }
  
  data.frame("MeanSignificant"=result,"MeanConfLevel"=1-alpha)
}

speedup.perform_median_test <- function(data1, data2, alpha, silent=FALSE) {
  failed <- FALSE				 
  result <- NA
  n1 <- length(data1)
  n2 <- length(data2)
  kt <- tryCatch(ks.test(data1-median(data1),data2-median(data2)),error=speedup.error_handler)
  if(is.null(kt)) {
    warn.msg(paste("An error occured while doing Kolmogorov statistical test:",speedup.errmsg,sep=""),silent=silent)
    failed <- TRUE
  } else {
    if(is.na(kt$p.value)) {
      warn.msg("Kolmogorov test was unable to compute a p-value.",silent=silent)
      failed <- TRUE
    } else {
      if(kt$p.value <= alpha) {
        if(n1 <= 30) {
          failed <- TRUE
          warn.msg("Sample1 too small for applying the Wilcoxon-Mann-Whitney's test (speedup of the median).  Please do more than 30 observations of the executions times.",silent=silent)
        } 
        if(n2 <= 30) {
          failed <- TRUE
          warn.msg("Sample2 too small for applying the Wilcoxon-Mann-Whitney's test (speedup of the median).  Please do more than 30 observations of the executions times.",silent=silent)
        } 
        if((n1 > 30) && (n2 > 30)) {
          warn.msg("The two samples do not fit the location shift model. The indicated confidence level for the speedup of the median execution time  may not be accurate.",silent=silent)
        }
      }
    }
  }
  if(!failed) {
    wt <- tryCatch(wilcox.test(data1, data2, alternative="greater"),error=speedup.error_handler)
    if(is.null(wt)) {
      warn.msg(paste("An error occured while doing Wilcoxon-Mann-Whitney statistical test:",speedup.errmsg,sep=""),silent=silent)
    } else {
      if(is.na(wt$p.value)) {
        warn.msg("Wilcoxon-Mann-Whitney test was unable to compute a p-value.",silent=silent)
      } else {
        if(wt$p.value <= alpha) {
          result <- TRUE
        } else {
          result <- FALSE
        }
      }
    }
  }
  return(result)
}

speedup.median_test <- function(bcfg, data1, data2) {
  if(is.na(config.level)) {
    level <- bcfg[1,"ConfLevel"]
    if((typeof(level) != "double") || (is.na(level))) {
      alpha <- NA
    } else {
      alpha <- 1 - level
    }
  } else {
    alpha <- 1 - config.level
  }
  if(is.na(alpha)) {
    x <- speedup.search_alpha(function(alpha) { speedup.perform_median_test(data1, data2, alpha, silent=TRUE) })
    result <- x$result
    alpha <- x$alpha
    if(is.na(result)) {
      speedup.perform_median_test(data1, data2, 0.5)
      warn.msg("Unable to find a confidence level greater than 50% to guarantee the statistical significance of median speedup.")
    } else {
      result <- speedup.perform_median_test(data1, data2, alpha)
    }
  } else {
    result <- speedup.perform_median_test(data1, data2, alpha)
  }
  if(is.na(result)) {
    result <- FALSE
  }
  
  data.frame("MedianSignificant"=result,"MedianConfLevel"=1-alpha)
}

speedup.round <- function(x) {
  round(x * 1000)/1000
}

speedup.perform_tests <- function(bcfg, data1, data2) {
  obs1 <- speedup.get_observation(data1)
  colnames(obs1) <- c("Min1","Max1","Mean1","Median1")
  obs2 <- speedup.get_observation(data2)
  colnames(obs2) <- c("Min2","Max2","Mean2","Median2")

  ## Min
  min_ratio <- speedup.safe_ratio(obs1[1,"Min1"],obs2[1,"Min2"])
  minres <- cbind("SpeedupMin"=speedup.round(min_ratio))

  ## Max
  max_ratio <- speedup.safe_ratio(obs1[1,"Max1"],obs2[1,"Max2"])
  maxres <- cbind("SpeedupMax"=speedup.round(max_ratio))

  ## Mean
  mean_ratio <- speedup.safe_ratio(obs1[1,"Mean1"],obs2[1,"Mean2"])
  mean_test <- speedup.mean_test(bcfg, data1, data2)
  meanres <- cbind("SpeedupMean"=speedup.round(mean_ratio),mean_test)

  ## Median
  median_ratio <- speedup.safe_ratio(obs1[1,"Median1"],obs2[1,"Median2"])
  median_test <- speedup.median_test(bcfg, data1, data2)
  medianres <- cbind("SpeedupMedian"=speedup.round(median_ratio),median_test)

  ##
  result <- cbind(obs1,obs2,minres,maxres,meanres,medianres)
  result
}

speedup.do_all_tests <- function(cfg) {
  result <- NULL

  bad <- c()

  totalW <- c(0,0,0,0)
  totalT <- c(0,0,0,0)
  gain <- c(0,0,0,0)

  n <- nrow(cfg)
  for(i in 1:n) {
    bcfg <- cfg[i,]
    bname <- as.character(bcfg[1,"Name"])

    ## enter warning
    warn.begin(bname)

    ## load first sample
    fname1 <- as.character(bcfg[1,"Sample1"])
    data1 <- speedup.load_bench(fname1)

    ## load second sample
    fname2 <- as.character(bcfg[1,"Sample2"])
    data2 <- speedup.load_bench(fname2)

    if(is.null(data1) || is.null(data2)) {
      warn.msg("Cannot process benchmark: samples unavailable.")
      bad <- c(bad, -i)
    } else {
      analysis <- cbind("Name"=bname,speedup.perform_tests(bcfg, data1,data2))
      if(config.weight == "custom") {
        w <- bcfg[1,"Coef"]
	weights <- cbind(w,w,w,w)
      } else if(config.weight == "equal") {
	weights <- cbind(1,1,1,1)
      } else {
	weights <- cbind(analysis[1,"Min1"],analysis[1,"Max1"],analysis[1,"Mean1"],analysis[1,"Median1"])
      }      
      colnames(weights) <- c("CoefMin","CoefMax","CoefMean","CoefMedian")

      W <- c(weights[1,"CoefMin"],weights[1,"CoefMax"],weights[1,"CoefMean"],weights[1,"CoefMedian"])
      x1 <- c(analysis[1,"Min1"],analysis[1,"Max1"],analysis[1,"Mean1"],analysis[1,"Median1"])
      x2 <- c(analysis[1,"Min2"],analysis[1,"Max2"],analysis[1,"Mean2"],analysis[1,"Median2"])

      gain <- gain + W * (x1 - x2)
      totalW <- totalW + W
      totalT <- totalT + W * x1      

      result <- rbind(result,cbind(analysis,weights))
    }

    ## exit warning
    warn.end()
  }

#    bench.cfg <- bench.cfg[bad,]

  gain <- gain / totalT
  gdata <- cbind("GainMin"=gain[1],"GainMax"=gain[2],"GainMean"=gain[3],"GainMedian"=gain[4])

  list(analysis=result,gain=gdata)
}

## compute the number of needed benchmark
## p/n = c = proportion of good samples
## alpha = desired confidence level
## r = desired precision
## return n
speedup.get.number.bench <- function(p, n, alpha, r) {
  z <- qnorm((1+alpha)/2)
  c <- p/n
  ceiling((z*z*c*(1-c)) / (r*r))
}

speedup.proportion <- function(analysis, name, coltest, collevel) {
  test <- analysis[[coltest]]
  levels <- analysis[[collevel]]

  if(is.na(config.level)) {
    lev <- 0.95
  } else {
    lev <- config.level
  }

  n <- length(test)
  p <- length(test[test])
  result.msg(paste("The observed proportion of accelerated benchmarks (speedup of the ",name,") a/b = ",p,"/",n," = ",speedup.round(p/n),sep=""))

  cint <- prop.test(p, n, conf.level=lev)


  result.msg(paste("The confidence level for computing proportion confidence interval is ",lev,".",sep=""))
  result.msg(paste("Proportion confidence interval (speedup of the ",name,") = [", speedup.round(cint$conf.int[1]), "; ", speedup.round(cint$conf.int[2]),"]",sep=""))
 if(p*(1-p/n) <= 5) {
    result.msg("Warning: this confidence interval of the proportion may not be accurate because the validity condition {a(1-a/b)>5} is not satisfied.")
   } 
  if(!is.na(config.precision)) {
    nb <- speedup.get.number.bench(p, n, lev, config.precision)
    if(nb > 0) {
      result.msg(paste("The minimal needed number of randomly selected benchmarks is ", nb ," (in order to have a precision r=",config.precision,").",sep=""))
    } #else {
      #result.msg(paste("Sufficiently enough benchmarks were performed."))
    #}
    result.msg("Remark: The computed confidence interval of the proportion is invalid if b the  experimented set of benchmarks is not randomly selected among a huge number of representative benchmarks.")
  }
}

## process a configuration file
## file = filename
## return a data frame
speedup.process <- function(file) {
  ## read the configuration file (and perform basic checks)
  bench.cfg <- speedup.read.config(file)

  result <- speedup.do_all_tests(bench.cfg)
  analysis <- result$analysis
  gain <- result$gain

  result.msg("")
  result.msg(paste("Overall gain (ExecutionTime=min) = ",speedup.round(gain[1,"GainMin"]),sep=""))
  result.msg(paste("Overall speedup (ExecutionTime=min) = ",speedup.round(1/(1-gain[1,"GainMin"])),sep=""))

#  result.msg("")
#  result.msg(paste("Overall gain (max) = ",speedup.round(gain[1,"GainMax"]),sep=""))
#  result.msg(paste("Overall speedup (max) = ",speedup.round(1/(1-gain[1,"GainMax"])),sep=""))

  result.msg("")
  result.msg(paste("Overall gain (ExecutionTime=mean) = ",speedup.round(gain[1,"GainMean"]),sep=""))
  result.msg(paste("Overall speedup (ExecutionTime=mean) = ",speedup.round(1/(1-gain[1,"GainMean"])),sep=""))

  result.msg("")
  result.msg(paste("Overall gain (ExecutionTime=median) = ",speedup.round(gain[1,"GainMedian"]),sep=""))
  result.msg(paste("Overall speedup (ExecutionTime=median) = ",speedup.round(1/(1-gain[1,"GainMedian"])),sep=""))

  result.msg("")
  speedup.proportion(analysis,"mean","MeanSignificant","MeanConfLevel")

  result.msg("")
  speedup.proportion(analysis,"median","MedianSignificant","MedianConfLevel")

  data <- subset(analysis,select=c("Name",
                           "SpeedupMin",
#			   "SpeedupMax",
                           "SpeedupMean","MeanSignificant","MeanConfLevel",
                           "SpeedupMedian","MedianSignificant","MedianConfLevel",
			   "CoefMin",
#			   "CoefMax",
			   "CoefMean",
			   "CoefMedian"))

  colnames(data) <- c("Name",
                      "SpeedupMin",
                      "SpeedupMean","IsMeanSignificant","MeanConfLevel",
                      "SpeedupMedian","IsMedianSignificant","MedianConfLevel",
                      "CoefMin","CoefMean","CoefMedian")
  data			   
}

##First read in the arguments listed at the command line
args=(commandArgs(TRUE))

##args is now a list of character vectors
## First check to see if arguments are passed.
## Then cycle through each element of the list and evaluate the expressions.
if(length(args)==0){
  stop("No arguments supplied.")
}else{
  for(i in 1:length(args)){
    eval(parse(text=args[[i]]))
  }

  result.msg(paste("Analysis report of ",config.ifile),append=FALSE)
  warn.open(paste("Warnings regarding analysis of ",config.ifile,sep=""))

  ## check that confidence level is between 0 and 1
  if(!is.na(config.level)) {
    if((config.level <= 0) || (config.level >= 1)) {
      stop("Illegal default confidence level: ",config.level, "\n")
      config.level <- NA
    } else {
      result.msg(paste("Default confidence level = ", config.level,sep=""))
    }
  }

  ## check that wanted precision for confidence interval is between 0 and 1
  if(!is.na(config.precision)) {
    if((config.precision < 0) || (config.precision > 1)) {
      stop("Illegal confidence interval precision: ", config.precision, "\n")
      config.precision <- NA
    } 
  } else {
    config.precision <- 0.05
  }

  if(is.na(config.ifile)) {
    stop("No filename provided.\n")
  } else {
    ## process the file
#    options(warn=-1)
    result <- speedup.process(config.ifile)
#    options(warn=0)
    if(!is.na(config.ofile)) {
      ## write CSV file
      write.csv(result, file=config.ofile,row.names=FALSE)
    }

    warn.close()
  }
}

