variable "region" {
  type        = string
  description = "The AWS region to deploy to."
}

variable "thing_name" {
  type        = string
  description = "The name of the Thing to be created."
}

variable "thing_type_name" {
  type        = string
  description = "The name of the Thing Type to be associated with the Thing."
}

variable "policy_name" {
  type        = string
  description = "The name of the Policy to be associated with the Thing."
}

variable "iot_endpoint" {
  type        = string
  description = "The endpoint of the AWS IoT Core service."
}
